/* SPDX-License-Identifier: GPL-3.0-or-later
 * ESP32 DevKit V1 fork (plan U6b): continuous decimated IQ stream (IQS1
 * frames, docs/iq-stream.md) for the classic ESP32, over UART.
 * Interface: iq_stream.h. Ring findings: docs/devkitv1/ring-feasibility.md.
 *
 * Ring. The capture writer runs as an 8192-word ring (control bit 17). DPORT
 * MAC_DUMP_MODE 2 maps its offsets 0..8191 to the lower 32 KiB of the window,
 * mode 1 to the upper 32 KiB; the CPU can read only the bank the RF does not
 * own. The app CPU, started bare metal (IDF stays unicore), switches the RF
 * between the banks. Unlike the U4 probe it does not switch at the pointer
 * wrap but every SESSION = 8192 - 2*ZW pairs, so each bank session covers
 * less than one lap and never overwrites its own start. Before a bank is
 * handed to the RF, the 3*ZW words around the session's planned start and end
 * are filled with a sentinel. Afterwards the sentinel run left between the
 * session's end and its start gives both switch points exactly: the session
 * holds stream pairs [P0, P1) at ring offsets P0 % 8192 .. P1 % 8192. A switch
 * hands the writer from one bank to the other at one write, so normally the
 * next session starts exactly where this one ended: no pair is lost. Any pair
 * that is missing anyway (late switch, lost DPORT write, inconsistent marks)
 * shows up as a jump between one session's P1 and the next delivered P0,
 * because ring offsets are tied to the absolute pair index.
 *
 * Work split. Reads of the CPU-owned bank are slow while the RF writes the
 * other one (about 10 cycles per pair on the bench against 6 with the RF
 * stopped), so the app CPU does only the ring and stage 1: a 16-pair boxcar
 * (CIC order 1; IQS mode 2 multiplies by j^n first) to 1 MS/s. It pushes the
 * block sums into a single-producer queue. Core 0 (FreeRTOS, interrupts on)
 * runs stage 2, a Kaiser-windowed FIR decimating by dec/16 (L = min(12*dec/16,
 * 256) taps, beta 5.65, cutoff at the output Nyquist), rounds >> shift and
 * saturates as the S3 does (output = 10-bit sample x 32), builds IQS1 frames
 * with CRC32 and drains them to the UART. Bits 1 (devkitv1 only) hard-limits
 * instead: each output keeps only the sign of I and of Q (shift is ignored),
 * four complex samples per byte, so FSK at 125 or 250 kS/s fits the UART.
 *
 * Core-0 load (bench, 2026-11-05, build with -DCMAKE_C_FLAGS=-DIQS_PROF, which
 * prints an IQSPROF line before IQSEND and self-checks dot16): stage 2 is about
 * 24 M MAC/s at every dec. With GCC's l16ui/mula dot product and one call per
 * block it took ~220 cycles per 1 MS/s block (~92 % of core 0; the queue
 * reached 7146 of 8192 at 4-bit dec 256 and overflowed at 1-bit dec 64/128).
 * The asm dot16 (32-bit loads, mula.aa.ll/hh, zero-overhead loop) and batched
 * drain give 77-119 cycles per block: 32 % (8-bit dec 512) to 50 % (1-bit dec
 * 64), queue peak ~1000.
 *
 * Loss semantics (same as the S3 engine): after any input gap the pipeline
 * realigns to the decimation grid, the next frame's sample_index jumps (time
 * stays exact at fs/dec) and flag bit 0 is set. IQSEND `abandoned` and `ffts`
 * both carry the input pairs missing from the ring or dropped because the
 * block queue was full (not the alignment skip), so they are a cross-check for
 * the index jumps, not an addition to them. `drops` counts frames lost to a
 * full output queue (flag bit 1, index jump). `detail` counts sessions
 * dropped because their switch marks were inconsistent (0 in a healthy run);
 * `late_max` is the worst switch lateness in pairs; `work_max` the worst
 * app-CPU cycles per session (the budget is 8064 x 15 = 120960); `units`
 * sessions; `pairs` input pairs delivered to the filter.
 *
 * App-CPU rules (ring-feasibility.md): the app CPU runs only IRAM code and
 * touches only internal DRAM (its cache SRAM is heap in unicore), with
 * interrupts masked. It only writes DPORT (CPU-3.10 errata: DPORT reads by
 * one CPU can corrupt the other CPU's 0x3ff4xxxx peripheral reads), and
 * writes the bank select twice in case of a lost access (CPU-3.16). The ROM
 * boot area of the app CPU is kept out of the heap.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_rom_crc.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/cpu_utility_ll.h"
#include "soc/soc.h"
#include "soc/dport_reg.h"
#include "soc/dport_access.h"
#include "heap_memory_layout.h"
#include "burst_serial.h"
#include "iq_stream.h"
#ifdef IQS_PROF /* bench only: core-0 load line before IQSEND (not part of the protocol) */
#include <stdio.h>
static uint64_t prof_busy;
static uint32_t prof_qmax;
static uint64_t prof_dot, prof_blocks;
static uint32_t prof_dot_bad;
#endif

#ifndef RING_PROBE /* dsp_probe.c reserves the same area in probe builds */
/* The app CPU's ROM boot path uses its ROM data and stack here; a unicore
 * build would otherwise hand this area to the heap. */
SOC_RESERVE_MEMORY_REGION(0x3ffe3f20, 0x3ffe8000, iqs_app_rom);
#endif

extern void ets_set_appcpu_boot_addr(uint32_t);
extern int _vector_table;

#define DUMP_CTRL    0x60033d90u
#define DUMP_STATUS  0x60033d94u
#define DUMP_BYTES   0x60033dc4u
#define WINDOW       ((volatile uint32_t *)0x3ffe8000)
#define BANK         8192u                /* ring words per bank */
#define BANK_MASK    (BANK - 1u)
#define ZW           64u                  /* mark zone half width, pairs */
#define SESSION      (BANK - 2u * ZW)     /* pairs between switches */
#define MARK         0xa5c33c5au          /* sentinel; not 0xa55aa55a (RF-owned reads) */
#define C1_DECIM     16u                  /* first stage */
#define BQ_N         8192u                /* block queue: 8 ms of 1 MS/s blocks */
#define SQ_N         64u                  /* segment marks */
#define IQS_MAGIC    0x31535149u          /* "IQS1" */
#define IQS_PAYLOAD  1024u
#define IQS_HDR      24u
#define SLOT_BYTES   (IQS_HDR + IQS_PAYLOAD + 4u)
#define NSLOT        8u
#define MAX_TAPS     256u
#define HIST_N       (MAX_TAPS + 512u)    /* d2 divides 768 - L: compaction only after an output */
_Static_assert((BQ_N & (BQ_N - 1u)) == 0 && (SQ_N & (SQ_N - 1u)) == 0, "queue sizes");

typedef struct __attribute__((packed)) {
    uint32_t magic, frame;
    uint64_t sample_index;
    uint16_t samples;
    uint8_t bits, flags;
    uint16_t dec;
    uint8_t gain, shift;
} iqs_header_t;
_Static_assert(sizeof(iqs_header_t) == IQS_HDR, "IQS header");

/* ---- shared state (internal DRAM; core 0 <-> app CPU) ------------------- */
typedef struct {
    volatile uint32_t alive, run, stop, done, beat, gain;
    volatile uint32_t bq_head, bq_tail; /* blocks pushed / consumed (free-running) */
    volatile uint32_t sq_head, sq_tail; /* segment marks pushed / consumed */
} c1_ctl_t;
static DRAM_ATTR c1_ctl_t c1;

typedef struct { /* a new segment: the block at queue position pos starts at pair index idx */
    uint32_t pos;
    uint64_t idx;
} seg_t;

typedef struct { /* run parameters, written by core 0 before c1.run++ */
    uint32_t sel[2], owner;  /* DPORT_IRAM_DRAM_AHB_SEL values: RF on bank 0/1, original */
    uint32_t rot, cyc_per_pair;
} c1_par_t;
static DRAM_ATTR c1_par_t par;

typedef struct { /* app-CPU results, read by core 0 after c1.done */
    uint32_t units, bad, late_max, work_max;
    uint64_t pairs, lost;
} c1_res_t;
static DRAM_ATTR c1_res_t res;

/* heap (BSS must stay below the RF window, sram_guard.ld) */
static uint32_t *bq;        /* BQ_N block sums: I low 16 bits, Q high 16 bits */
static seg_t *sq;           /* SQ_N segment marks */
static uint8_t *slots;      /* NSLOT x SLOT_BYTES, finished frames (core 0 only) */
static int16_t *taps;       /* MAX_TAPS, Q15, sum 32768 */
static int16_t *hist;       /* 2 x HIST_N: I then Q, linear, newest at f.n - 1 */
static uint8_t *cur;        /* SLOT_BYTES, frame being built */

/* ---- app CPU: ring and stage 1 ------------------------------------------ */
typedef struct {
    uint64_t next_in;    /* pair index the stream continues from */
    bool synced, seg;    /* next_in valid; next block starts a new segment */
    int32_t ai, aq;      /* stage 1 partial block */
    uint32_t n1;         /* pairs in the partial block */
} pipe_t;
static DRAM_ATTR pipe_t pp;

static IRAM_ATTR inline void c1_memw(void) {
    __asm__ volatile("memw" ::: "memory");
}

/* One block sum into the queue; first pair index a (multiple of 16). */
static IRAM_ATTR inline __attribute__((always_inline)) void blk_push(int32_t si, int32_t sq_, uint64_t a) {
    uint32_t h = c1.bq_head;
    if (h - c1.bq_tail >= BQ_N || (pp.seg && c1.sq_head - c1.sq_tail >= SQ_N)) {
        res.lost += C1_DECIM; /* core 0 behind: drop, start a new segment later */
        pp.seg = true;
        return;
    }
    if (pp.seg) {
        uint32_t s = c1.sq_head;
        sq[s % SQ_N].pos = h;
        sq[s % SQ_N].idx = a;
        c1_memw();
        c1.sq_head = s + 1u;
        pp.seg = false;
    }
    bq[h % BQ_N] = ((uint32_t)si & 0xffffu) | ((uint32_t)sq_ << 16);
    c1_memw();
    c1.bq_head = h + 1u;
}

/* One pair into the partial block; a = its pair index. */
static IRAM_ATTR void stage1_one(uint32_t w, uint64_t a) {
    int32_t i = (int32_t)(w << 22) >> 22, q = (int32_t)(w << 12) >> 22;
    if (par.rot) { /* z * j^n */
        uint32_t phase = (uint32_t)a & 3u;
        int32_t t = i;
        if (phase == 1) {
            i = -q;
            q = t;
        } else if (phase == 2) {
            i = -i;
            q = -q;
        } else if (phase == 3) {
            i = q;
            q = -t;
        }
    }
    pp.ai += i;
    pp.aq += q;
    if (++pp.n1 == C1_DECIM) {
        blk_push(pp.ai, pp.aq, a + 1u - C1_DECIM);
        pp.ai = pp.aq = 0;
        pp.n1 = 0;
    }
}

/* Whole aligned blocks from p; a = pair index of p[0]. */
static IRAM_ATTR __attribute__((noinline)) void stage1_blocks(const volatile uint32_t *vp, uint32_t blocks,
                                                              uint64_t a) {
    const uint32_t *p = (const uint32_t *)vp;
    if (!par.rot) {
        for (uint32_t b = 0; b < blocks; b++, p += C1_DECIM, a += C1_DECIM) {
            int32_t si = 0, sq_ = 0;
#pragma GCC unroll 16
            for (uint32_t j = 0; j < C1_DECIM; j++) {
                uint32_t w = p[j];
                si += (int32_t)(w << 22) >> 22;
                sq_ += (int32_t)(w << 12) >> 22;
            }
            blk_push(si, sq_, a);
        }
        return;
    }
    for (uint32_t b = 0; b < blocks; b++, p += C1_DECIM, a += C1_DECIM) {
        int32_t si = 0, sq_ = 0;
#pragma GCC unroll 16
        for (uint32_t j = 0; j < C1_DECIM; j++) { /* z * j^n, n = j & 3 (block starts at n = 0) */
            uint32_t w = p[j];
            int32_t i = (int32_t)(w << 22) >> 22, q = (int32_t)(w << 12) >> 22;
            if ((j & 3u) == 0) {
                si += i;
                sq_ += q;
            } else if ((j & 3u) == 1) {
                si -= q;
                sq_ += i;
            } else if ((j & 3u) == 2) {
                si -= i;
                sq_ -= q;
            } else {
                si += q;
                sq_ -= i;
            }
        }
        blk_push(si, sq_, a);
    }
}

/* n contiguous ring pairs at p (no bank wrap), first pair index a. */
static IRAM_ATTR void stage1_run(const volatile uint32_t *p, uint64_t a, uint32_t n) {
    while (n && (pp.n1 || ((uint32_t)a & (C1_DECIM - 1u)))) {
        stage1_one(*p++, a++);
        n--;
    }
    uint32_t blocks = n / C1_DECIM;
    if (blocks) {
        stage1_blocks(p, blocks, a);
        p += blocks * C1_DECIM;
        a += blocks * C1_DECIM;
        n -= blocks * C1_DECIM;
    }
    while (n) {
        stage1_one(*p++, a++);
        n--;
    }
}

/* Stream pairs [p0, p1) from bank b into stage 1. */
static IRAM_ATTR void deliver(unsigned b, uint64_t p0, uint64_t p1) {
    bool align = !pp.synced;
    if (pp.synced && p0 != pp.next_in) {
        if (p0 > pp.next_in) {
            res.lost += p0 - pp.next_in;
            align = true;
        } else { /* overlap: already have these */
            if (pp.next_in >= p1) return;
            p0 = pp.next_in;
        }
    }
    if (align) { /* drop the partial block; restart on the 16-pair grid */
        pp.ai = pp.aq = 0;
        pp.n1 = 0;
        pp.seg = true;
        uint64_t s = (p0 + C1_DECIM - 1u) & ~(uint64_t)(C1_DECIM - 1u);
        if (s >= p1) { /* nothing usable in this session */
            pp.synced = false;
            res.lost += p1 - p0;
            return;
        }
        p0 = s;
    }
    pp.synced = true;
    pp.next_in = p1;
    const volatile uint32_t *base = WINDOW + b * BANK;
    c1.gain = base[(uint32_t)p0 & BANK_MASK] >> 20;
    res.pairs += p1 - p0;
    while (p0 < p1) {
        uint32_t off = (uint32_t)p0 & BANK_MASK, n = BANK - off;
        if ((uint64_t)n > p1 - p0) n = (uint32_t)(p1 - p0);
        stage1_run(base + off, p0, n);
        p0 += n;
    }
}

static IRAM_ATTR void mark_zone(unsigned b, uint32_t start) {
    volatile uint32_t *w = WINDOW + b * BANK;
    for (uint32_t k = 0; k < 3u * ZW; k++) w[(start + k) & BANK_MASK] = MARK;
}

/* Bank b just finished the session planned for pairs [a0, a1), a1 = a0 + SESSION.
 * Its mark zone starts at a1 % BANK: written tail, then marks, then written head. */
static IRAM_ATTR void finish_session(unsigned b, uint64_t a0, uint64_t a1) {
    const volatile uint32_t *w = WINDOW + b * BANK;
    uint32_t zs = (uint32_t)a1 & BANK_MASK, first = ~0u, last = ~0u;
    for (uint32_t k = 0; k < 3u * ZW; k++) {
        if (w[(zs + k) & BANK_MASK] == MARK) {
            if (first == ~0u) first = k;
            last = k;
        }
    }
    bool ok = first < 2u * ZW && last + 1u >= 2u * ZW && last + 1u < 3u * ZW;
    for (uint32_t k = first; ok && k <= last; k++) ok = w[(zs + k) & BANK_MASK] == MARK;
    if (ok)
        deliver(b, a0 + (last + 1u - 2u * ZW), a1 + first);
    else
        res.bad++;
    mark_zone(b, ((uint32_t)a1 + 2u * SESSION) & BANK_MASK); /* for its next session */
}

static IRAM_ATTR inline void select_bank(uint32_t v) {
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, v);
    c1_memw();
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, v); /* CPU-3.16: an access may be lost */
    c1_memw();
}

static IRAM_ATTR void c1_stream(void) {
    pp.next_in = 0;
    pp.synced = pp.seg = false;
    pp.ai = pp.aq = 0;
    pp.n1 = 0;
    res.units = res.bad = res.late_max = res.work_max = 0;
    res.pairs = res.lost = 0;
    const uint32_t cpp = par.cyc_per_pair;
    REG_WRITE(DUMP_CTRL, 0);
    select_bank(par.sel[0]);
    uint32_t ctrl = BIT(16) | BANK | BIT(17); /* 16 MS/s, 8192-word ring */
    REG_WRITE(DUMP_CTRL, ctrl | BIT(18));
    REG_WRITE(DUMP_CTRL, ctrl);
    REG_WRITE(DUMP_CTRL, ctrl | BIT(31));
    uint32_t last_ptr = 0, last_cc = esp_cpu_get_cycle_count();
    uint64_t abs_ptr = 0, a0 = 0, a1 = SESSION;
    unsigned rf = 0;
    while (!c1.stop) {
        /* Track the write pointer as an absolute pair index; CCOUNT settles
         * the lap count should a poll come more than half a lap late. */
        for (;;) {
            uint32_t p = REG_READ(DUMP_STATUS) & BANK_MASK, cc = esp_cpu_get_cycle_count();
            uint32_t d = (p - last_ptr) & BANK_MASK, est = (cc - last_cc) / cpp;
            if (est > BANK / 2u) d += ((est - d + BANK / 2u) / BANK) * BANK;
            abs_ptr += d;
            last_ptr = p;
            last_cc = cc;
            if (abs_ptr >= a1 || c1.stop) break;
            uint32_t t = esp_cpu_get_cycle_count();
            while (esp_cpu_get_cycle_count() - t < 64u) {} /* spare the peripheral bus */
        }
        if (c1.stop) break;
        select_bank(par.sel[rf ^ 1u]);
        uint32_t late = (uint32_t)(abs_ptr - a1);
        if (late > res.late_max) res.late_max = late;
        uint32_t t0 = esp_cpu_get_cycle_count();
        finish_session(rf, a0, a1);
        uint32_t work = esp_cpu_get_cycle_count() - t0;
        if (work > res.work_max) res.work_max = work;
        res.units++;
        c1.beat++;
        rf ^= 1u;
        /* A late switch only lengthens this session; if it lapped its own
         * start the marks are gone and finish_session drops it (counted).
         * The schedule stays fixed, so a late loop catches up by itself. */
        a0 = a1;
        a1 += SESSION;
    }
    REG_WRITE(DUMP_CTRL, 0);
    uint32_t t = esp_cpu_get_cycle_count();
    while (esp_cpu_get_cycle_count() - t < 2400u) {} /* 10 us */
    select_bank(par.owner);
}

IRAM_ATTR __attribute__((noreturn)) void iqs_c1_main(void) {
    uint32_t seen = c1.run;
    c1_memw();
    c1.alive = 1;
    for (;;) {
        while (c1.run == seen) {}
        seen = c1.run;
        c1_memw();
        c1_stream();
        c1_memw();
        c1.done = 1;
    }
}

/* Bare-metal app-CPU entry (as dsp_probe.c / s3_core1.S): own stack, vector
 * table for window exceptions, interrupts masked. */
__asm__("    .section .iram1.iqs_c1_entry, \"ax\"\n"
        "    .literal_position\n"
        "    .align 4\n"
        "    .global iqs_c1_entry\n"
        "    .type iqs_c1_entry, @function\n"
        "iqs_c1_entry:\n"
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
        "    movi a1, iqs_c1_stack_top\n"
        "    mov a2, a1\n"
        "    addi a1, a1, -32\n"
        "    s32e a2, a1, -12\n"
        "    movi a0, 0\n"
        "    movi a8, iqs_c1_main\n"
        "    callx8 a8\n"
        "1:  j 1b\n"
        "    .size iqs_c1_entry, . - iqs_c1_entry\n"
        "    .section .bss.iqs_c1_stack, \"aw\", @nobits\n"
        "    .align 16\n"
        "    .global iqs_c1_stack_top\n"
        "iqs_c1_stack:\n"
        "    .space 4096\n"
        "iqs_c1_stack_top:\n"
        "    .text\n");
extern void iqs_c1_entry(void);

/* ---- core 0: stage 2, frames, UART -------------------------------------- */
static struct {
    uint32_t dec, log2dec, d2, taps, bits, shift;
    uint32_t n, n2;               /* stage 2 history fill, inputs since the last output */
    uint32_t skip;                /* blocks to skip to reach the dec grid */
    uint64_t out_index, frame_index;
    uint32_t fill, fsamp, frame, frames, drops; /* payload bytes, samples in the frame */
    uint8_t flags;
    uint32_t s_head, s_tail;      /* frame slots */
    uint16_t s_len[NSLOT];
    uint32_t ms;
} f;

static void frame_flush(void) {
    if (!f.fill) return;
    iqs_header_t h = {.magic = IQS_MAGIC,
                      .frame = f.frame,
                      .sample_index = f.frame_index,
                      .samples = (uint16_t)f.fsamp,
                      .bits = (uint8_t)f.bits,
                      .flags = f.flags,
                      .dec = (uint16_t)f.dec,
                      .gain = (uint8_t)c1.gain,
                      .shift = (uint8_t)f.shift};
    if (f.s_head - f.s_tail < NSLOT) {
        uint8_t *s = slots + (f.s_head % NSLOT) * SLOT_BYTES;
        uint32_t len = IQS_HDR + f.fill;
        memcpy(cur, &h, sizeof(h));
        memcpy(s, cur, len);
        uint32_t crc = esp_rom_crc32_le(0, s, len);
        memcpy(s + len, &crc, 4);
        f.s_len[f.s_head % NSLOT] = (uint16_t)(len + 4u);
        f.s_head++;
        f.frames++;
        f.frame++;
        f.flags = 0;
    } else {
        f.drops++;
        f.flags |= 2;
    }
    f.frame_index = f.out_index;
    f.fill = 0;
    f.fsamp = 0;
}

static void iqs_put(int32_t i, int32_t q) {
    if (f.bits == 1) { /* sign bits: sample k of a byte has I at bit 2k, Q at bit 2k+1; 1 = negative */
        uint32_t k = f.fsamp & 3u, v = ((uint32_t)i >> 31) | (((uint32_t)q >> 31) << 1);
        if (k == 0) cur[IQS_HDR + f.fill++] = (uint8_t)v;
        else cur[IQS_HDR + f.fill - 1u] |= (uint8_t)(v << (2u * k));
        f.fsamp++;
        f.out_index++;
        if (f.fsamp >= 4u * IQS_PAYLOAD) frame_flush();
        return;
    }
    uint32_t sh = f.shift;
    if (sh) { /* round to nearest */
        int32_t r = 1 << (sh - 1);
        i = (i + r) >> sh;
        q = (q + r) >> sh;
    }
    int32_t lim = (1 << (f.bits - 1)) - 1;
    if (i > lim) i = lim;
    else if (i < -lim - 1) i = -lim - 1;
    if (q > lim) q = lim;
    else if (q < -lim - 1) q = -lim - 1;
    uint8_t *o = cur + IQS_HDR + f.fill;
    if (f.bits == 8) {
        o[0] = (uint8_t)i;
        o[1] = (uint8_t)q;
        f.fill += 2;
    } else if (f.bits == 16) {
        o[0] = (uint8_t)i;
        o[1] = (uint8_t)(i >> 8);
        o[2] = (uint8_t)q;
        o[3] = (uint8_t)(q >> 8);
        f.fill += 4;
    } else {
        o[0] = (uint8_t)((i & 15) | (q << 4));
        f.fill += 1;
    }
    f.fsamp++;
    f.out_index++;
    if (f.fill >= IQS_PAYLOAD) frame_flush();
}

/* Sum x[k] * h[k], k < L. x and h are 4-byte aligned and L is a multiple of 8
 * (L = 12 * d2 or 256, n - L even): two 32-bit loads feed two MAC16 steps
 * (mula.aa.ll / mula.aa.hh), in a zero-overhead loop. Same result as the plain
 * C loop; about 2x fewer instructions than GCC's l16ui/mula code. */
static __attribute__((noinline)) int32_t dot16(const int16_t *x, const int16_t *h, uint32_t L) {
    int32_t r;
    uint32_t t0, t1, t2, t3, n = L >> 3;
    const int16_t *px = x, *ph = h;
    __asm__ volatile("movi %[t0], 0\n"
                     "wsr %[t0], acclo\n"
                     "wsr %[t0], acchi\n"
                     "loopnez %[n], 1f\n"
                     "l32i %[t0], %[x], 0\n"
                     "l32i %[t1], %[h], 0\n"
                     "l32i %[t2], %[x], 4\n"
                     "l32i %[t3], %[h], 4\n"
                     "mula.aa.ll %[t0], %[t1]\n"
                     "mula.aa.hh %[t0], %[t1]\n"
                     "l32i %[t0], %[x], 8\n"
                     "l32i %[t1], %[h], 8\n"
                     "mula.aa.ll %[t2], %[t3]\n"
                     "mula.aa.hh %[t2], %[t3]\n"
                     "l32i %[t2], %[x], 12\n"
                     "l32i %[t3], %[h], 12\n"
                     "mula.aa.ll %[t0], %[t1]\n"
                     "mula.aa.hh %[t0], %[t1]\n"
                     "addi %[x], %[x], 16\n"
                     "addi %[h], %[h], 16\n"
                     "mula.aa.ll %[t2], %[t3]\n"
                     "mula.aa.hh %[t2], %[t3]\n"
                     "1:\n"
                     "rsr %[r], acclo\n"
                     : [r] "=r"(r), [x] "+r"(px), [h] "+r"(ph), [n] "+r"(n), [t0] "=&r"(t0), [t1] "=&r"(t1),
                       [t2] "=&r"(t2), [t3] "=&r"(t3)
                     :
                     : "memory");
    return r;
}

/* New segment starting at pair index idx (multiple of 16). */
static void stage2_realign(uint64_t idx) {
    frame_flush();
    for (uint32_t k = 0; k < 2u * HIST_N; k++) hist[k] = 0;
    f.n = f.taps;
    f.n2 = 0;
    uint64_t s = (idx + f.dec - 1u) & ~(uint64_t)(f.dec - 1u);
    f.skip = (uint32_t)((s - idx) / C1_DECIM);
    f.out_index = f.frame_index = s >> f.log2dec;
    if (f.frame || f.frames || f.drops) f.flags |= 1;
}

/* Blocks bq[t .. t+cnt) of one segment through stage 2 (locals in registers;
 * the per-block cost matters: 1 M blocks/s on core 0). */
static void stage2_run(uint32_t t, uint32_t cnt) {
    while (cnt && f.skip) {
        f.skip--;
        t++;
        cnt--;
    }
    int16_t *xi = hist, *xq = hist + HIST_N;
    uint32_t n = f.n, n2 = f.n2;
    const uint32_t d2 = f.d2, L = f.taps;
    for (; cnt; cnt--, t++) {
        uint32_t v = bq[t % BQ_N];
        xi[n] = (int16_t)(v & 0xffffu);
        xq[n] = (int16_t)(v >> 16);
        n++;
        if (++n2 < d2) continue;
        n2 = 0;
#ifdef IQS_PROF
        uint32_t pc = esp_cpu_get_cycle_count();
#endif
        /* taps sum 32768, input 16 x 10-bit: >> 14 gives the 10-bit sample x 32 */
        int32_t ai = dot16(xi + n - L, taps, L), aq = dot16(xq + n - L, taps, L);
        iqs_put(ai >> 14, aq >> 14);
#ifdef IQS_PROF
        prof_dot += esp_cpu_get_cycle_count() - pc;
#endif
        if (n == HIST_N) {
            memcpy(xi, xi + n - L, 2u * L);
            memcpy(xq, xq + n - L, 2u * L);
            n = L;
        }
    }
    f.n = n;
    f.n2 = n2;
}

/* Consume up to max queued blocks; returns how many. */
static uint32_t stage2_drain(uint32_t max) {
    uint32_t t = c1.bq_tail, h = c1.bq_head, done = 0;
    c1_memw();
    while (t != h && done < max) {
        uint32_t end = h - t > max - done ? t + (max - done) : h;
        if (c1.sq_tail != c1.sq_head) {
            c1_memw();
            seg_t *s = &sq[c1.sq_tail % SQ_N];
            if (s->pos == t) {
                stage2_realign(s->idx);
                c1.sq_tail = c1.sq_tail + 1u;
                continue; /* the next mark (if any) is at a later block */
            }
            if (s->pos - t < end - t) end = s->pos; /* stop before the next segment */
        }
        stage2_run(t, end - t);
        done += end - t;
        t = end;
        c1_memw();
        c1.bq_tail = t;
    }
    return done;
}

static bool c1_booted;
static bool c1_start(void) {
    if (c1_booted) return c1.alive != 0;
    c1_booted = true;
    esp_cpu_unstall(1);
    cpu_utility_ll_enable_clock_and_reset_app_cpu();
    ets_set_appcpu_boot_addr((uint32_t)iqs_c1_entry);
    int64_t t0 = esp_timer_get_time();
    while (!c1.alive && esp_timer_get_time() - t0 < 50000) {}
    return c1.alive != 0;
}

static float kaiser_i0(float x) {
    float s = 1.0f, t = 1.0f;
    for (int k = 1; k < 30; k++) {
        float q = x / (2.0f * k);
        t *= q * q;
        s += t;
    }
    return s;
}

/* Kaiser (beta 5.65) windowed sinc, cutoff at the output Nyquist, Q15, sum 32768. */
static void make_taps(uint32_t L, uint32_t d2) {
    float fc = 0.5f / (float)d2, sum = 0.0f, h[MAX_TAPS];
    for (uint32_t i = 0; i < L; i++) {
        float m = (float)i - 0.5f * (float)(L - 1), x = 2.0f * fc * m;
        float sinc = fabsf(x) < 1e-6f ? 1.0f : sinf((float)M_PI * x) / ((float)M_PI * x);
        float r = 2.0f * m / (float)(L - 1);
        h[i] = sinc * kaiser_i0(5.65f * sqrtf(fmaxf(0.0f, 1.0f - r * r)));
        sum += h[i];
    }
    int32_t total = 0;
    for (uint32_t i = 0; i < L; i++) {
        taps[i] = (int16_t)lrintf(32768.0f * h[i] / sum);
        total += taps[i];
    }
    taps[L / 2] += (int16_t)(32768 - total); /* exact unity DC gain */
}

bool iq_stream_start(unsigned ms, unsigned dec, unsigned bits, unsigned rate_code, unsigned shift, unsigned mode) {
    if (rate_code != 6 || dec < 64 || dec > 1024 || (dec & (dec - 1)) || (bits != 1 && bits != 4 && bits != 8 && bits != 16) ||
        (mode != 0 && mode != 2))
        return false;
    if (!slots) {
        const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
        bq = heap_caps_malloc(BQ_N * sizeof(uint32_t), caps);
        sq = heap_caps_malloc(SQ_N * sizeof(seg_t), caps);
        slots = heap_caps_malloc(NSLOT * SLOT_BYTES, caps);
        taps = heap_caps_malloc(MAX_TAPS * sizeof(int16_t), caps);
        hist = heap_caps_malloc(2u * HIST_N * sizeof(int16_t), caps);
        cur = heap_caps_malloc(SLOT_BYTES, caps);
        if (!bq || !sq || !slots || !taps || !hist || !cur) {
            heap_caps_free(bq);
            heap_caps_free(sq);
            heap_caps_free(slots);
            heap_caps_free(taps);
            heap_caps_free(hist);
            heap_caps_free(cur);
            bq = NULL;
            sq = NULL;
            slots = cur = NULL;
            taps = hist = NULL;
            return false;
        }
    }
    uint32_t d2 = dec / C1_DECIM, L = 12u * d2;
    if (L > MAX_TAPS) L = MAX_TAPS;
#ifdef IQS_PROF /* dot16 (asm) against the plain C sum on pseudo-random data, every even offset */
    {
        uint32_t x = 12345u;
        for (uint32_t k = 0; k < 2u * HIST_N; k++) {
            x = x * 1103515245u + 12345u;
            hist[k] = (int16_t)(x >> 16);
        }
        for (uint32_t k = 0; k < MAX_TAPS; k++) {
            x = x * 1103515245u + 12345u;
            taps[k] = (int16_t)(x >> 16);
        }
        prof_dot_bad = 0;
        for (uint32_t Lt = 8; Lt <= MAX_TAPS; Lt += 8)
            for (uint32_t o = 0; o + Lt <= 2u * HIST_N; o += 34u) {
                int32_t ref = 0;
                for (uint32_t k = 0; k < Lt; k++) ref += hist[o + k] * taps[k];
                if (dot16(hist + o, taps, Lt) != ref) prof_dot_bad++;
            }
    }
#endif
    make_taps(L, d2);
    memset(&f, 0, sizeof(f));
    f.dec = dec;
    f.log2dec = (uint32_t)__builtin_ctz(dec);
    f.d2 = d2;
    f.taps = L;
    f.bits = bits;
    f.shift = shift;
    f.ms = ms;
    par.rot = mode == 2;
    par.cyc_per_pair = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ / 16u;
    return c1_start();
}

void iq_stream_run_until_stop(iq_stream_stats_t *out) {
    memset(out, 0, sizeof(*out));
    REG_WRITE(DUMP_CTRL, 0);
    for (uint32_t j = 0; j < 2u * BANK; j++) WINDOW[j] = MARK; /* window is CPU-owned here */
    uint32_t owner = DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    uint32_t base = owner & ~DPORT_MAC_DUMP_MODE_M;
    par.sel[0] = base | (2u << DPORT_MAC_DUMP_MODE_S);
    par.sel[1] = base | (1u << DPORT_MAC_DUMP_MODE_S);
    par.owner = owner;
    uint32_t byte_select = REG_READ(DUMP_BYTES);
    REG_WRITE(DUMP_BYTES, 0x03020100); /* as acquire_iq / mac_init */
    c1.bq_head = c1.bq_tail = c1.sq_head = c1.sq_tail = 0;
    c1.stop = 0;
    c1.done = 0;
    c1.beat = 0;
    c1_memw();
    c1.run++;
    int64_t t0 = esp_timer_get_time(), last_ok = t0, last_beat_t = t0;
    uint32_t last_beat = 0, sent = 0;
    bool host = false, stalled = false, blocked = false;
    for (;;) {
        int64_t now = esp_timer_get_time();
        if (f.ms && now - t0 >= (int64_t)f.ms * 1000) break;
        if (burst_serial_stop_requested()) {
            host = true;
            break;
        }
        uint32_t beat = c1.beat;
        if (beat != last_beat) {
            last_beat = beat;
            last_beat_t = now;
        } else if (now - last_beat_t > 200000) {
            stalled = true;
            break;
        }
#ifdef IQS_PROF
        uint32_t pq = c1.bq_head - c1.bq_tail, pc = esp_cpu_get_cycle_count();
        if (pq > prof_qmax) prof_qmax = pq;
#endif
        uint32_t work = stage2_drain(512);
#ifdef IQS_PROF
        prof_blocks += work;
        prof_busy += esp_cpu_get_cycle_count() - pc;
#endif
        bool progress = false;
        while (f.s_tail != f.s_head) {
            uint8_t *s = slots + (f.s_tail % NSLOT) * SLOT_BYTES;
            uint32_t len = f.s_len[f.s_tail % NSLOT];
            size_t n = burst_serial_try_send(s + sent, len - sent);
            if (!n) break;
            progress = true;
            last_ok = now;
            sent += n;
            if (sent < len) break;
            sent = 0;
            f.s_tail++;
        }
        if (!f.ms && f.s_tail != f.s_head && now - last_ok > 2000000) { /* nobody reads: end the run */
            host = blocked = true;
            break;
        }
        /* Sleep a tick only when idle: 1 ms is < 1.4 ms of UART FIFO and 1000
         * blocks of the 8192-block queue. */
        if (!progress && !work) vTaskDelay(1);
    }
#ifdef IQS_PROF
    {
        char l[160];
        int64_t el = esp_timer_get_time() - t0;
        uint64_t cyc = (uint64_t)CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * (uint64_t)(el ? el : 1);
        int n = snprintf(l, sizeof l, "IQSPROF dot_bad %u stage2_busy_permille %u dot_put_permille %u blocks %u cyc_per_block %u qmax %u of %u\n",
                         (unsigned)prof_dot_bad, (unsigned)(prof_busy * 1000u / cyc), (unsigned)(prof_dot * 1000u / cyc), (unsigned)prof_blocks,
                         (unsigned)(prof_blocks ? prof_busy / prof_blocks : 0), (unsigned)prof_qmax, (unsigned)BQ_N);
        prof_dot = prof_blocks = 0;
        burst_serial_send(l, (size_t)n);
        prof_busy = 0;
        prof_qmax = 0;
    }
#endif
    c1_memw();
    c1.stop = 1;
    int64_t ts = esp_timer_get_time();
    while (!c1.done && esp_timer_get_time() - ts < 100000) {}
    bool done = c1.done != 0;
    REG_WRITE(DUMP_CTRL, 0);
    esp_rom_delay_us(10);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, owner);
    REG_WRITE(DUMP_BYTES, byte_select);
    /* Finish the queued blocks and drain the frames, unless the host stopped reading. */
    if (done && !blocked) {
        bool ok = true;
        while (ok && stage2_drain(4096)) {
            while (f.s_tail != f.s_head) {
                uint32_t len = f.s_len[f.s_tail % NSLOT];
                if (!burst_serial_send(slots + (f.s_tail % NSLOT) * SLOT_BYTES + sent, len - sent)) { ok = false; break; }
                sent = 0;
                f.s_tail++;
            }
        }
        if (ok) frame_flush();
        while (ok && f.s_tail != f.s_head) {
            uint32_t len = f.s_len[f.s_tail % NSLOT];
            if (!burst_serial_send(slots + (f.s_tail % NSLOT) * SLOT_BYTES + sent, len - sent)) break;
            sent = 0;
            f.s_tail++;
        }
    }
    out->status = (stalled || !done) ? 2u : 0u; /* app CPU stopped making progress */
    out->detail = done ? res.bad : 0u;
    out->units = res.units;
    out->pairs = res.pairs;
    out->elapsed_us = (uint64_t)(esp_timer_get_time() - t0);
    out->late_max = res.late_max;
    out->work_max = res.work_max;
    out->frames = f.frames;
    out->drops = f.drops;
    out->abandoned = (uint32_t)res.lost;
    out->ffts = (uint32_t)res.lost;
    out->stopped_by_host = host;
}
