/* SPDX-License-Identifier: GPL-3.0-or-later
 * ESP32 DevKit V1 fork (plan U4.3): DSP timing probe on the app CPU.
 * Built only with -DRING_PROBE=ON; never packaged.
 *
 * IDF stays CONFIG_FREERTOS_UNICORE. DSPPROBE starts the app CPU bare metal,
 * as the S3 backend does with c1_start (main/common/ring_capture.c), and
 * times a first-stage decimate-by-16 boxcar over a capture in the sample
 * window. The app CPU runs only IRAM code with interrupts masked: in unicore
 * mode its flash cache SRAM belongs to the IRAM heap, so it must never touch
 * flash. Its entry sets its own stack, like s3_core1.S.
 *
 *   DSPPROBE rate      rate code 0/1/6; captures 16380 samples first
 * reply: DSPPROBE rate n=<pairs> c1=<0|1> then one DSPK line per kernel:
 *   DSPK <name> c1=<cycles> pro=<cycles> c1_x100=<cycles/pair*100> pro_x100=.. match=<0|1>
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "hal/cpu_utility_ll.h"
#include "soc/soc.h"
#include "heap_memory_layout.h"

/* The ROM boot path of the app CPU uses its ROM data and stack area, which a
 * unicore build hands to the heap. Keep it out of the heap in probe builds. */
SOC_RESERVE_MEMORY_REGION(0x3ffe3f20, 0x3ffe8000, dsp_probe_app_rom);

extern void ets_set_appcpu_boot_addr(uint32_t);
extern int _vector_table;

#define DECIM 16
#define NPAIRS 16368u /* 16380 captured, rounded down to a multiple of 16 */
#define NOUT (NPAIRS / DECIM)

typedef struct {
    volatile uint32_t alive, cmd, kernel, cycles;
    const void *volatile in;
    int32_t *volatile out;
} c1_mail_t;
static DRAM_ATTR c1_mail_t c1m;

__asm__(
    "    .section .iram1.dsp_c1_entry, \"ax\"\n"
    "    .literal_position\n"
    "    .align 4\n"
    "    .global dsp_c1_entry\n"
    "    .type dsp_c1_entry, @function\n"
    "dsp_c1_entry:\n"
    "    movi a2, 0\n"
    "    wsr a2, intenable\n"
    "    wsr a2, windowbase\n"
    "    movi a2, 1\n"
    "    wsr a2, windowstart\n"
    "    rsync\n"
    "    movi a2, _vector_table\n"
    "    wsr a2, vecbase\n"
    "    movi a2, 0x4002f\n" /* WOE, UM, INTLEVEL 15 */
    "    wsr a2, ps\n"
    "    rsync\n"
    "    movi a1, dsp_c1_stack_top\n"
    "    mov a2, a1\n"
    "    addi a1, a1, -32\n"
    "    s32e a2, a1, -12\n"
    "    movi a0, 0\n"
    "    movi a8, dsp_c1_main\n"
    "    callx8 a8\n"
    "1:  j 1b\n"
    "    .size dsp_c1_entry, . - dsp_c1_entry\n"
    "    .section .bss.dsp_c1_stack, \"aw\", @nobits\n"
    "    .align 16\n"
    "    .global dsp_c1_stack_top\n"
    "dsp_c1_stack:\n"
    "    .space 4096\n"
    "dsp_c1_stack_top:\n"
    "    .text\n");
extern void dsp_c1_entry(void);

/* Kernels are compiled -O2 with the 16-tap inner loop unrolled, as a real
 * first stage would be; the rest of the component stays at the IDF default. */
#define KOPT __attribute__((noinline, optimize("O2")))

/* v0: raw 32-bit words, sign-extend the 10-bit I [9:0] and Q [19:10]. */
static IRAM_ATTR KOPT void k_raw_sext(const uint32_t *in, int32_t *out) {
    for (unsigned k = 0; k < NOUT; k++) {
        int32_t si = 0, sq = 0;
#pragma GCC unroll 16
        for (unsigned j = 0; j < DECIM; j++) {
            uint32_t w = in[j];
            si += (int32_t)(w << 22) >> 22;
            sq += (int32_t)(w << 12) >> 22;
        }
        in += DECIM;
        out[2 * k] = si;
        out[2 * k + 1] = sq;
    }
}

/* v1: raw words to offset binary (flip both sign bits), masked accumulate. */
static IRAM_ATTR KOPT void k_raw_mask(const uint32_t *in, int32_t *out) {
    for (unsigned k = 0; k < NOUT; k++) {
        uint32_t ai = 0, aq = 0;
#pragma GCC unroll 16
        for (unsigned j = 0; j < DECIM; j++) {
            uint32_t w = in[j] ^ 0x80200u;
            ai += w & 0x3ffu;
            aq += w & 0xffc00u;
        }
        in += DECIM;
        out[2 * k] = (int32_t)ai - 512 * DECIM;
        out[2 * k + 1] = (int32_t)(aq >> 10) - 512 * DECIM;
    }
}

/* v2: packed int8 I/Q pairs (the CAP16 wire format). */
static IRAM_ATTR KOPT void k_int8(const int8_t *in, int32_t *out) {
    for (unsigned k = 0; k < NOUT; k++) {
        int32_t si = 0, sq = 0;
#pragma GCC unroll 16
        for (unsigned j = 0; j < DECIM; j++) {
            si += in[2 * j];
            sq += in[2 * j + 1];
        }
        in += 2 * DECIM;
        out[2 * k] = si;
        out[2 * k + 1] = sq;
    }
}

/* v3: offset-binary words; Q from (sum of words - sum of I fields) >> 10. */
static IRAM_ATTR KOPT void k_raw_sum(const uint32_t *in, int32_t *out) {
    for (unsigned k = 0; k < NOUT; k++) {
        uint32_t aw = 0, ai = 0;
#pragma GCC unroll 16
        for (unsigned j = 0; j < DECIM; j++) {
            uint32_t w = in[j] ^ 0x80200u;
            aw += w;
            ai += w & 0x3ffu;
        }
        in += DECIM;
        out[2 * k] = (int32_t)ai - 512 * DECIM;
        out[2 * k + 1] = (int32_t)(((aw - ai) >> 10) & 0x3fffu) - 512 * DECIM;
    }
}

/* v4: memory baseline, one 32-bit load and add per pair. */
static IRAM_ATTR KOPT void k_sum(const uint32_t *in, int32_t *out) {
    for (unsigned k = 0; k < NOUT; k++) {
        uint32_t a = 0;
#pragma GCC unroll 16
        for (unsigned j = 0; j < DECIM; j++) a += in[j];
        in += DECIM;
        out[2 * k] = (int32_t)a;
        out[2 * k + 1] = 0;
    }
}

static IRAM_ATTR uint32_t run_kernel(unsigned kernel, const void *in, int32_t *out) {
    uint32_t t0 = esp_cpu_get_cycle_count();
    if (kernel == 0)
        k_raw_sext(in, out);
    else if (kernel == 1)
        k_raw_mask(in, out);
    else if (kernel == 2)
        k_int8(in, out);
    else if (kernel == 3)
        k_raw_sum(in, out);
    else
        k_sum(in, out);
    return esp_cpu_get_cycle_count() - t0;
}

IRAM_ATTR __attribute__((noreturn)) void dsp_c1_main(void) {
    c1m.alive = 1;
    for (;;) {
        while (!c1m.cmd) {}
        c1m.cycles = run_kernel(c1m.kernel, c1m.in, c1m.out);
        __asm__ volatile("memw");
        c1m.cmd = 0;
    }
}

static bool c1_started;
static bool c1_start(void) {
    if (c1_started) return c1m.alive != 0;
    c1_started = true;
    esp_cpu_unstall(1);
    cpu_utility_ll_enable_clock_and_reset_app_cpu();
    ets_set_appcpu_boot_addr((uint32_t)dsp_c1_entry);
    int64_t t0 = esp_timer_get_time();
    while (!c1m.alive && esp_timer_get_time() - t0 < 50000) {}
    return c1m.alive != 0;
}

static uint32_t c1_run(unsigned kernel, const void *in, int32_t *out) {
    c1m.kernel = kernel;
    c1m.in = in;
    c1m.out = out;
    __asm__ volatile("memw");
    c1m.cmd = 1;
    int64_t t0 = esp_timer_get_time();
    while (c1m.cmd && esp_timer_get_time() - t0 < 100000) {}
    return c1m.cmd ? 0 : c1m.cycles;
}

static bool send_text(bool (*send)(const void *, size_t), const char *text) {
    return send(text, strlen(text));
}

/* Provided by the ESP32 receiver through ring_probe.h. */
bool dsp_probe_command(const char *line, bool (*send)(const void *, size_t));
extern bool esp32_dsp_capture(unsigned rate, const uint32_t **data);

bool dsp_probe_command(const char *line, bool (*send)(const void *, size_t)) {
    unsigned rate;
    char extra;
    if (sscanf(line, "DSPPROBE %u %c", &rate, &extra) != 1) return false;
    if (rate != 0 && rate != 1 && rate != 6) {
        send_text(send, "ERR args\n");
        return true;
    }
    const uint32_t *raw;
    if (!esp32_dsp_capture(rate, &raw)) return true;
    int8_t *packed = heap_caps_malloc(2 * NPAIRS, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    int32_t *out_c1 = heap_caps_malloc(2 * NOUT * sizeof(int32_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    int32_t *out_pro = heap_caps_malloc(2 * NOUT * sizeof(int32_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!packed || !out_c1 || !out_pro) {
        send_text(send, "ERR dsp_memory\n");
        goto done;
    }
    for (unsigned j = 0; j < NPAIRS; j++) {
        packed[2 * j] = (int8_t)((raw[j] >> 2) & 255);
        packed[2 * j + 1] = (int8_t)((raw[j] >> 12) & 255);
    }
    bool ok = c1_start();
    char text[192];
    snprintf(text, sizeof(text), "DSPPROBE %u n=%u decim=%u c1=%u\n", rate, NPAIRS, DECIM, ok);
    send_text(send, text);
    static const char *const names[5] = {"raw_sext", "raw_mask", "int8", "raw_sum", "load_add"};
    for (unsigned k = 0; k < 5; k++) {
        const void *in = k == 2 ? (const void *)packed : (const void *)raw;
        uint32_t best_c1 = ~0u, best_pro = ~0u;
        for (unsigned r = 0; r < 4; r++) {
            if (ok) {
                uint32_t c = c1_run(k, in, out_c1);
                if (c && c < best_c1) best_c1 = c;
            }
            unsigned irq = portSET_INTERRUPT_MASK_FROM_ISR();
            uint32_t c = run_kernel(k, in, out_pro);
            portCLEAR_INTERRUPT_MASK_FROM_ISR(irq);
            if (c < best_pro) best_pro = c;
        }
        bool match = ok && !memcmp(out_c1, out_pro, 2 * NOUT * sizeof(int32_t));
        snprintf(text, sizeof(text), "DSPK %s c1=%lu pro=%lu c1_x100=%lu pro_x100=%lu match=%u out0=%ld,%ld\n",
                 names[k], (unsigned long)best_c1, (unsigned long)best_pro,
                 ok && best_c1 != ~0u ? (unsigned long)((uint64_t)best_c1 * 100 / NPAIRS) : 0ul,
                 (unsigned long)((uint64_t)best_pro * 100 / NPAIRS), match, (long)out_pro[0], (long)out_pro[1]);
        send_text(send, text);
    }
done:
    heap_caps_free(packed);
    heap_caps_free(out_c1);
    heap_caps_free(out_pro);
    return true;
}
