/* SPDX-License-Identifier: GPL-3.0-or-later
 * ESP32 DevKit V1 fork: hop-following receive during an IQS stream (receive only).
 * Included by receiver.c after tune_rx(); design and measurements in
 * experiments/14-hop-follow/notes.md, retune method in experiments/10-fast-retune/notes.md (mode 1).
 *
 * Before IQS (ordinary commands):
 *   HOPTAB i khz [khz ...]   LO frequencies (absolute kHz, the LO itself: the host
 *                            subtracts the mode-2 IF) for table slots i, i+1, ...
 *   HOPCLR                   empty table (IQS then stops on any host byte, as before)
 *   HOPTAB?                  reply HOPTAB n
 * With a table loaded, an IQS run reads host lines (an empty line stops it):
 *   HS slot pair period_mp   from stream pair index `pair` on, retune to table slot
 *                            `slot`, then to the next slot every period_mp/1000 pairs
 *                            (16 MS/s pairs; 8.994 ms = 143904000). A start in the
 *                            past is advanced by whole periods (and slots).
 *   HA dpairs                shift the schedule by dpairs (signed)
 *   HP period_mp             change the period from the next hop on
 *   HX slot                  stop the schedule and retune to `slot` now
 * There are no replies during the run; every retune tags the stream (iq_stream.c:
 * flag bit 2 starts a channel segment, the shift byte is the slot). After the run
 * the next tune is a full one.
 */
#include "esp_cpu.h"

#define HOPF_MAX 64
static unsigned hopf_khz[HOPF_MAX], hopf_n;
static esp_timer_handle_t hopf_timer;
static portMUX_TYPE hopf_mux = portMUX_INITIALIZER_UNLOCKED;
static struct {
    bool on;
    unsigned slot;          /* slot of the next hop */
    uint64_t next_mp;       /* next hop, milli-pairs */
    uint64_t period_mp;
} hs;
static volatile uint32_t hopf_hops, hopf_late_max, hopf_skipped, hopf_qfull, hopf_rt_max;

static void hopf_retune(unsigned slot) {
    unsigned khz = hopf_khz[slot];
    unsigned xtal = rtc_clk_xtal_freq_get();
    uint64_t ps = iq_stream_pair_now();
    unsigned cap = ram_chip_i2c_readReg(0x62, 1, 0);
    ram_chip_i2c_writeReg(0x62, 1, 0, cap & ~0x80u);
    rom_set_rf_freq_offset(xtal == 26 ? 1 : xtal == 24 ? 2 : 0, khz / 1000u,
                           (int)(((khz % 1000u) * 1024u + 500u) / 1000u));
    uint64_t pe = iq_stream_pair_now();
    if ((uint32_t)(pe - ps) > hopf_rt_max) hopf_rt_max = (uint32_t)(pe - ps);
    if (!iq_stream_hop_mark(ps, pe, slot)) hopf_qfull++;
    hopf_hops++;
}

/* Arm the timer for hs.next_mp (call with hopf_mux held). */
static void hopf_arm_locked(uint64_t now) {
    uint64_t next = hs.next_mp / 1000u;
    int64_t us = next > now ? (int64_t)((next - now) / 16u) : 0;
    esp_timer_stop(hopf_timer);
    esp_timer_start_once(hopf_timer, us > 0 ? (uint64_t)us : 1u);
}

static void hopf_cb(void *arg) {
    (void)arg;
    portENTER_CRITICAL(&hopf_mux);
    if (!hs.on) {
        portEXIT_CRITICAL(&hopf_mux);
        return;
    }
    uint64_t now = iq_stream_pair_now(), next = hs.next_mp / 1000u;
    if (next > now + 16u * 100u) { /* woke more than 100 us early: wait again */
        hopf_arm_locked(now);
        portEXIT_CRITICAL(&hopf_mux);
        return;
    }
    /* Missed hops (late start or a long stall): skip whole periods, keep the slot sequence. */
    while (now > next && now - next > hs.period_mp / 2000u) {
        hs.next_mp += hs.period_mp;
        hs.slot = (hs.slot + 1u) % hopf_n;
        next = hs.next_mp / 1000u;
        hopf_skipped++;
    }
    if (next > now + 16u * 100u) {
        hopf_arm_locked(now);
        portEXIT_CRITICAL(&hopf_mux);
        return;
    }
    if (now > next && (uint32_t)(now - next) > hopf_late_max) hopf_late_max = (uint32_t)(now - next);
    unsigned slot = hs.slot;
    hs.slot = (hs.slot + 1u) % hopf_n;
    hs.next_mp += hs.period_mp;
    portEXIT_CRITICAL(&hopf_mux);
    hopf_retune(slot); /* ~0.26 ms of ROM/I2C work: outside the critical section */
    portENTER_CRITICAL(&hopf_mux);
    if (hs.on) hopf_arm_locked(iq_stream_pair_now());
    portEXIT_CRITICAL(&hopf_mux);
}

static bool hopf_parse_i64(const char *s, int64_t *out) {
    while (*s == ' ') s++;
    bool neg = *s == '-';
    if (*s == '-' || *s == '+') s++;
    if (*s < '0' || *s > '9') return false;
    uint64_t v = 0;
    while (*s >= '0' && *s <= '9') v = v * 10u + (uint64_t)(*s++ - '0');
    while (*s == ' ') s++;
    if (*s) return false;
    *out = neg ? -(int64_t)v : (int64_t)v;
    return true;
}

/* In-stream line (iq_stream run loop, main task). Malformed lines are ignored. */
static void hopf_line(const char *line) {
    if (!strncmp(line, "HS ", 3)) {
        char *e;
        unsigned long slot = strtoul(line + 3, &e, 10);
        unsigned long long pair = strtoull(e, &e, 10), per = strtoull(e, &e, 10);
        if (slot >= hopf_n || per < 16000000ull || per > 1600000000ull) return;
        portENTER_CRITICAL(&hopf_mux);
        hs.slot = (unsigned)slot;
        hs.next_mp = (uint64_t)pair * 1000u;
        hs.period_mp = per;
        hs.on = true;
        hopf_arm_locked(iq_stream_pair_now());
        portEXIT_CRITICAL(&hopf_mux);
    } else if (!strncmp(line, "HA ", 3)) {
        int64_t d;
        if (!hopf_parse_i64(line + 3, &d)) return;
        portENTER_CRITICAL(&hopf_mux);
        if (hs.on) {
            hs.next_mp = (uint64_t)((int64_t)hs.next_mp + d * 1000);
            hopf_arm_locked(iq_stream_pair_now());
        }
        portEXIT_CRITICAL(&hopf_mux);
    } else if (!strncmp(line, "HP ", 3)) {
        int64_t p;
        if (!hopf_parse_i64(line + 3, &p) || p < 16000000 || p > 1600000000) return;
        portENTER_CRITICAL(&hopf_mux);
        hs.period_mp = (uint64_t)p;
        portEXIT_CRITICAL(&hopf_mux);
    } else if (!strncmp(line, "HX ", 3)) {
        unsigned long slot = strtoul(line + 3, NULL, 10);
        if (slot >= hopf_n) return;
        portENTER_CRITICAL(&hopf_mux);
        hs.on = false;
        esp_timer_stop(hopf_timer);
        portEXIT_CRITICAL(&hopf_mux);
        hopf_retune((unsigned)slot);
    }
}

/* Around an IQS run: on before (only with a table), off after. */
static void hopf_run_begin(void) {
    if (!hopf_n) return;
    if (!hopf_timer) {
        const esp_timer_create_args_t a = {.callback = hopf_cb, .name = "hop"};
        if (esp_timer_create(&a, &hopf_timer) != ESP_OK) return;
    }
    hs.on = false;
    hopf_hops = hopf_late_max = hopf_skipped = hopf_qfull = hopf_rt_max = 0;
    iq_stream_set_line_hook(hopf_line);
}

static void hopf_run_end(void) {
    if (!hopf_timer) return;
    portENTER_CRITICAL(&hopf_mux);
    hs.on = false;
    portEXIT_CRITICAL(&hopf_mux);
    esp_timer_stop(hopf_timer);
    iq_stream_set_line_hook(NULL);
    rx_ready = false; /* the LO is wherever the last hop left it */
}

static bool hopf_command(const char *line) {
    if (!strcmp(line, "HOPCLR")) {
        hopf_n = 0;
        reply("OK\n");
        return true;
    }
    if (!strcmp(line, "HOPTAB?")) {
        reply("HOPTAB %u\n", hopf_n);
        return true;
    }
    if (!strcmp(line, "HOPSTAT?")) {
        reply("HOPSTAT hops %u late_max_pairs %u skipped %u qfull %u retune_max_pairs %u\n", (unsigned)hopf_hops,
              (unsigned)hopf_late_max, (unsigned)hopf_skipped, (unsigned)hopf_qfull, (unsigned)hopf_rt_max);
        return true;
    }
    if (strncmp(line, "HOPTAB ", 7)) return false;
    unsigned v[16];
    int k = burst_parse_uints(line + 7, v, 16);
    if (k < 2 || v[0] + (unsigned)(k - 1) > HOPF_MAX) {
        reply("ERR args\n");
        return true;
    }
    for (int j = 1; j < k; j++)
        if (v[j] < 2300000u || v[j] > 2560000u) {
            reply("ERR args\n");
            return true;
        }
    for (int j = 1; j < k; j++) hopf_khz[v[0] + j - 1] = v[j];
    if (v[0] + (unsigned)(k - 1) > hopf_n) hopf_n = v[0] + (unsigned)(k - 1);
    reply("OK\n");
    return true;
}
