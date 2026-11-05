/* Bounded ring-mode investigation, never enabled in distributed firmware.
 * Uses only the existing reserved capture aperture. No additional SRAM banks
 * are handed to RF. The writer is stopped before returning to the scheduler. */
#ifdef RING_PROBE
#include "esp_cpu.h"
#if CONFIG_IDF_TARGET_ESP32
/* Modified by Matthew Banaag, 2026: ESP32 split/ping-pong/re-arm/DSP probes (plan U4). */
#define RP_PATTERN 0xa5c33c5au
/* RINGSPLIT rate mode count: fill the 64 KiB window with a pattern, start the
 * writer in ring mode with MAC_DUMP_MODE=mode, and take six CPU snapshots of
 * 16 spaced words per 16 KiB quarter while it runs (1 ms). Then stop, give the
 * window back and count rewritten words per quarter. A quarter whose snapshot
 * hash changes between snapshots is visible to the CPU while RF writes it. */
static bool esp32_ring_split(const char *line) {
    unsigned rate, mode, count;
    char extra;
    if (sscanf(line, "RINGSPLIT %u %u %u %c", &rate, &mode, &count, &extra) != 3) return false;
    if ((rate != 0 && rate != 1 && rate != 6) || mode > 3 || count < 256 || count > 16384) {
        reply("ERR args\n");
        return true;
    }
    const uint32_t *data;
    unsigned elapsed;
    if (!spectrum_acquire(256, rate, &data, &elapsed)) return true;
    volatile uint32_t *w = (volatile uint32_t *)samples;
    for (unsigned j = 0; j < CAPACITY; j++) w[j] = RP_PATTERN;
    uint32_t owner = DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    uint32_t select = (owner & ~DPORT_MAC_DUMP_MODE_M) | (mode << DPORT_MAC_DUMP_MODE_S);
    uint32_t bits = rate == 6 ? BIT(16) : rate == 1 ? BIT(15) : 0;
    static uint32_t snap_hash[6][4], snap_chg[6][4], snap_ptr[6], snap_w0[6][4];
    unsigned irq = portSET_INTERRUPT_MASK_FROM_ISR();
    REG_WRITE(DUMP_CTRL, 0);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, select);
    (void)DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    uint32_t ctrl = bits | count | BIT(17);
    REG_WRITE(DUMP_CTRL, ctrl | BIT(18));
    REG_WRITE(DUMP_CTRL, ctrl);
    unsigned start = esp_cpu_get_cycle_count();
    REG_WRITE(DUMP_CTRL, ctrl | BIT(31));
    const unsigned span = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * 1000u;
    for (unsigned s = 0; s < 6; s++) {
        while (esp_cpu_get_cycle_count() - start < (s + 1) * span / 7) {}
        snap_ptr[s] = REG_READ(DUMP_STATUS) & 0x1ffffu;
        for (unsigned q = 0; q < 4; q++) {
            uint32_t h = 0, c = 0;
            for (unsigned k = 0; k < 16; k++) {
                uint32_t v = w[q * 4096 + k * 256 + 17];
                h = ((h << 5) | (h >> 27)) ^ v;
                c += v != RP_PATTERN;
            }
            snap_hash[s][q] = h;
            snap_chg[s][q] = c;
            snap_w0[s][q] = w[q * 4096 + 17];
        }
    }
    while (esp_cpu_get_cycle_count() - start < span) {}
    REG_WRITE(DUMP_CTRL, 0);
    esp_rom_delay_us(10);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, owner);
    (void)DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(irq);
    unsigned post[4] = {0};
    for (unsigned j = 0; j < CAPACITY; j++) post[j / 4096] += w[j] != RP_PATTERN;
    reply("RINGSPLIT %u %u %u\n", rate, mode, count);
    for (unsigned s = 0; s < 6; s++)
        reply("SNAP %u ptr=%lu q0=%lu/%08lx/%08lx q1=%lu/%08lx/%08lx q2=%lu/%08lx/%08lx q3=%lu/%08lx/%08lx\n", s,
              (unsigned long)snap_ptr[s], (unsigned long)snap_chg[s][0], (unsigned long)snap_hash[s][0],
              (unsigned long)snap_w0[s][0], (unsigned long)snap_chg[s][1], (unsigned long)snap_hash[s][1],
              (unsigned long)snap_w0[s][1], (unsigned long)snap_chg[s][2], (unsigned long)snap_hash[s][2],
              (unsigned long)snap_w0[s][2], (unsigned long)snap_chg[s][3], (unsigned long)snap_hash[s][3],
              (unsigned long)snap_w0[s][3]);
    reply("POST %u %u %u %u\n", post[0], post[1], post[2], post[3]);
    return true;
}

/* RINGREARM rate n reps: back-to-back one-shot captures of n samples, re-armed
 * by the CPU as acquire_iq does, all in MAC_DUMP_MODE 3 with interrupts masked.
 * Reports total cycles; gap per capture = total/reps - n/fs. */
static bool esp32_ring_rearm(const char *line) {
    unsigned rate, n, reps;
    char extra;
    if (sscanf(line, "RINGREARM %u %u %u %c", &rate, &n, &reps, &extra) != 3) return false;
    if ((rate != 0 && rate != 1 && rate != 6) || n < 256 || n > 16384 || !reps || reps > 1000) {
        reply("ERR args\n");
        return true;
    }
    const uint32_t *data;
    unsigned elapsed;
    if (!spectrum_acquire(256, rate, &data, &elapsed)) return true;
    unsigned clock = rate == 6 ? 0 : rate == 1 ? 2 : 1;
    uint32_t ctrl = n | ((clock == 0) << 16) | ((clock == 2) << 15);
    uint32_t owner = DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    unsigned irq = portSET_INTERRUPT_MASK_FROM_ISR();
    REG_WRITE(DUMP_CTRL, 0);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, (owner & ~DPORT_MAC_DUMP_MODE_M) | (3 << DPORT_MAC_DUMP_MODE_S));
    unsigned done = 0, maxwait = 0, minwait = ~0u, timeouts = 0;
    unsigned t0 = esp_cpu_get_cycle_count(), prev_done = t0;
    for (unsigned r = 0; r < reps; r++) {
        REG_WRITE(DUMP_CTRL, ctrl | BIT(31));
        REG_SET_BIT(DUMP_CTRL, BIT(19));
        REG_CLR_BIT(DUMP_CTRL, BIT(19));
        unsigned s = esp_cpu_get_cycle_count();
        while (!(REG_READ(DUMP_CTRL) & BIT(18)) && esp_cpu_get_cycle_count() - s < 2400000u) {}
        unsigned e = esp_cpu_get_cycle_count();
        if (REG_READ(DUMP_CTRL) & BIT(18))
            done++;
        else
            timeouts++;
        unsigned period = e - prev_done;
        if (r && period > maxwait) maxwait = period;
        if (r && period < minwait) minwait = period;
        prev_done = e;
        REG_WRITE(DUMP_CTRL, 0);
    }
    unsigned total = esp_cpu_get_cycle_count() - t0;
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, owner);
    (void)DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(irq);
    reply("RINGREARM %u %u %u done=%u timeouts=%u total_cycles=%u min_period=%u max_period=%u mhz=%u\n", rate, n,
          reps, done, timeouts, total, minwait, maxwait, (unsigned)CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    return true;
}
/* RINGPP rate halves margin: ping-pong test. The writer runs in ring mode over
 * the full 16384 words without stopping. MAC_DUMP_MODE follows its pointer:
 * 2 (MAC has the lower 32 KiB) while it writes the lower half, 1 (upper)
 * while it writes the upper half, and 3 (both) for +-margin words around each
 * half boundary, so no switch happens at the write position. Each time the
 * CPU gets a half back, it counts the words still holding the pattern and
 * refills it (every stride-th word). A CPU-readable ring shows 8192/stride written
 * words per half; it is gapless when late=0 and total_cycles = halves * 8192 / fs. */
static bool esp32_ring_pingpong(const char *line) {
    unsigned rate, halves, margin, stride = 1;
    char extra;
    int fields = sscanf(line, "RINGPP %u %u %u %u %c", &rate, &halves, &margin, &stride, &extra);
    if (fields != 3 && fields != 4) return false;
    if ((rate != 0 && rate != 1 && rate != 6) || !halves || halves > 20000 || margin < 16 || margin > 4000 ||
        !stride || stride > 4096 || (stride & (stride - 1))) {
        reply("ERR args\n");
        return true;
    }
    const uint32_t *data;
    unsigned elapsed;
    if (!spectrum_acquire(256, rate, &data, &elapsed)) return true;
    volatile uint32_t *w = (volatile uint32_t *)samples;
    for (unsigned j = 0; j < CAPACITY; j++) w[j] = RP_PATTERN;
    uint32_t owner = DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    uint32_t base = owner & ~DPORT_MAC_DUMP_MODE_M;
    uint32_t bits = rate == 6 ? BIT(16) : rate == 1 ? BIT(15) : 0;
    const unsigned half = CAPACITY / 2;
    unsigned done = 0, full = 0, minw = ~0u, maxw = 0, late = 0, first_w = 0, leftover = 0, timeout = 0;
    unsigned max_busy = 0;
    unsigned irq = portSET_INTERRUPT_MASK_FROM_ISR();
    REG_WRITE(DUMP_CTRL, 0);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, base | (2 << DPORT_MAC_DUMP_MODE_S));
    uint32_t ctrl = bits | CAPACITY | BIT(17);
    REG_WRITE(DUMP_CTRL, ctrl | BIT(18));
    REG_WRITE(DUMP_CTRL, ctrl);
    REG_WRITE(DUMP_CTRL, ctrl | BIT(31));
    unsigned t_all = esp_cpu_get_cycle_count();
#define RP_PTR() (REG_READ(DUMP_STATUS) & 0x3fffu)
#define RP_WAIT(cond)                                                                                    \
    do {                                                                                                 \
        unsigned t_ = esp_cpu_get_cycle_count();                                                         \
        unsigned o;                                                                                      \
        while (o = RP_PTR(), !(cond))                                                                    \
            if (esp_cpu_get_cycle_count() - t_ > 240000u) {                                              \
                timeout++;                                                                               \
                goto out;                                                                                \
            }                                                                                            \
    } while (0)
    /* cpu_half: the half the CPU just got back (0 lower, 1 upper). */
    for (unsigned h = 0; h < halves; h++) {
        unsigned cpu_half = h & 1 ? 0 : 1; /* h even: writer in lower, CPU has upper */
        if (h) {
            unsigned t0 = esp_cpu_get_cycle_count();
            volatile uint32_t *p = w + cpu_half * half;
            unsigned written = 0;
            for (unsigned j = 0; j < half; j += stride) {
                written += p[j] != RP_PATTERN;
                p[j] = RP_PATTERN;
            }
            unsigned busy = esp_cpu_get_cycle_count() - t0;
            if (busy > max_busy) max_busy = busy;
            if (h == 1)
                first_w = written;
            else {
                done++;
                full += written == half / stride;
                leftover += half / stride - written;
                if (written < minw) minw = written;
                if (written > maxw) maxw = written;
            }
        }
        /* Writer is in half (1 - cpu_half); hand over at its far boundary. */
        unsigned lo = cpu_half ? half - margin : CAPACITY - margin;
        unsigned hi = cpu_half ? half + margin : margin;
        {
            unsigned o0 = RP_PTR();
            bool in_writer_half = cpu_half ? o0 < half : o0 >= half;
            if (!in_writer_half || (cpu_half ? o0 >= lo : o0 >= lo)) late++;
        }
        if (cpu_half)
            RP_WAIT(o >= lo && o < half + half / 2);
        else
            RP_WAIT(o >= lo);
        DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, base | (3 << DPORT_MAC_DUMP_MODE_S));
        if (cpu_half)
            RP_WAIT(o >= hi && o < half + half / 2);
        else
            RP_WAIT(o >= hi && o < half / 2);
        /* Writer now in cpu_half: give that half to MAC, take the other. */
        DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, base | ((cpu_half ? 1u : 2u) << DPORT_MAC_DUMP_MODE_S));
        (void)DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    }
out:;
    unsigned total = esp_cpu_get_cycle_count() - t_all;
    REG_WRITE(DUMP_CTRL, 0);
    esp_rom_delay_us(10);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, owner);
    (void)DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(irq);
#undef RP_WAIT
#undef RP_PTR
    reply("RINGPP %u halves=%u margin=%u stride=%u checked=%u full=%u minw=%u maxw=%u leftover=%u first=%u late=%u "
          "timeout=%u max_busy=%u total_cycles=%u\n",
          rate, halves, margin, stride, done, full, done ? minw : 0, maxw, leftover, first_w, late, timeout, max_busy, total);
    return true;
}
/* RINGPP8 rate halves: ping-pong with an 8192-word ring. MAC_DUMP_MODE 1 and 2
 * map writer offsets 0..8191 to the upper and lower 32 KiB, and drop offsets
 * >= 8192 (see RINGPP), so the mode is toggled 2 <-> 1 at each pointer wrap,
 * detected by polling. Samples written before the switch lands go to the
 * start of the bank just finished; the new bank then keeps the pattern in its
 * first k words. When the CPU gets a bank back it counts pattern words in
 * words 0..63 (k, the per-switch loss) and every 64th word after (should be 0),
 * then refills those. Keep halves * 8192 / fs under ~250 ms (interrupt WDT). */
static bool esp32_ring_pp8(const char *line) {
    unsigned rate, halves;
    char extra;
    if (sscanf(line, "RINGPP8 %u %u %c", &rate, &halves, &extra) != 2) return false;
    if ((rate != 0 && rate != 1 && rate != 6) || halves < 3 || halves > 4000) {
        reply("ERR args\n");
        return true;
    }
    const uint32_t *data;
    unsigned elapsed;
    if (!spectrum_acquire(256, rate, &data, &elapsed)) return true;
    volatile uint32_t *w = (volatile uint32_t *)samples;
    for (unsigned j = 0; j < CAPACITY; j++) w[j] = RP_PATTERN;
    uint32_t owner = DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    uint32_t base = owner & ~DPORT_MAC_DUMP_MODE_M;
    const uint32_t sel[2] = {base | (2 << DPORT_MAC_DUMP_MODE_S), base | (1 << DPORT_MAC_DUMP_MODE_S)};
    uint32_t bits = rate == 6 ? BIT(16) : rate == 1 ? BIT(15) : 0;
    const unsigned half = CAPACITY / 2;
    unsigned kmin = ~0u, kmax = 0, ksum = 0, sparse = 0, overrun = 0, amin = ~0u, amax = 0, checked = 0;
    unsigned bmax = 0, timeout = 0;
    unsigned irq = portSET_INTERRUPT_MASK_FROM_ISR();
    REG_WRITE(DUMP_CTRL, 0);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, sel[0]);
    uint32_t ctrl = bits | half | BIT(17);
    REG_WRITE(DUMP_CTRL, ctrl | BIT(18));
    REG_WRITE(DUMP_CTRL, ctrl);
    REG_WRITE(DUMP_CTRL, ctrl | BIT(31));
    unsigned t_all = esp_cpu_get_cycle_count();
    unsigned bank = 0; /* bank the MAC currently writes: 0 lower, 1 upper */
    for (unsigned h = 0; h < halves; h++) {
        unsigned prev = REG_READ(DUMP_STATUS) & 0x3fffu, cur, t0 = esp_cpu_get_cycle_count();
        while ((cur = REG_READ(DUMP_STATUS) & 0x3fffu) >= prev) {
            prev = cur;
            if (esp_cpu_get_cycle_count() - t0 > 480000u) {
                timeout++;
                goto out;
            }
        }
        DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, sel[bank ^ 1]);
        unsigned after = REG_READ(DUMP_STATUS) & 0x3fffu;
        unsigned done_bank = bank;
        bank ^= 1;
        if (h >= 2) { /* the bank's lap started after a polled switch */
            if (after < amin) amin = after;
            if (after > amax) amax = after;
        }
        unsigned tb = esp_cpu_get_cycle_count();
        volatile uint32_t *p = w + done_bank * half;
        unsigned k = 0, s = 0;
        for (unsigned j = 0; j < 64; j++) {
            k += p[j] == RP_PATTERN;
            p[j] = RP_PATTERN;
        }
        for (unsigned j = 64; j < half; j += 64) {
            s += p[j] == RP_PATTERN;
            p[j] = RP_PATTERN;
        }
        unsigned busy = esp_cpu_get_cycle_count() - tb;
        if (busy > bmax) bmax = busy;
        if ((REG_READ(DUMP_STATUS) & 0x3fffu) < after) overrun++;
        if (h >= 2) { /* first two laps include the start-up */
            checked++;
            ksum += k;
            sparse += s;
            if (k < kmin) kmin = k;
            if (k > kmax) kmax = k;
        }
    }
out:;
    unsigned total = esp_cpu_get_cycle_count() - t_all;
    REG_WRITE(DUMP_CTRL, 0);
    esp_rom_delay_us(10);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, owner);
    (void)DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(irq);
    reply("RINGPP8 %u halves=%u checked=%u k_min=%u k_max=%u k_sum=%u sparse_missing=%u after_min=%u after_max=%u "
          "overrun=%u timeout=%u max_busy=%u total_cycles=%u\n",
          rate, halves, checked, checked ? kmin : 0, kmax, ksum, sparse, amin == ~0u ? 0 : amin, amax, overrun,
          timeout, bmax, total);
    return true;
}
extern bool dsp_probe_command(const char *line, bool (*send)(const void *, size_t));
/* For dsp_probe.c: a full one-shot capture into the window, CPU-owned on return. */
bool esp32_dsp_capture(unsigned rate, const uint32_t **data) {
    unsigned elapsed;
    return spectrum_acquire(MAX_SAMPLES, rate, data, &elapsed);
}
#endif
static bool ring_probe_command(const char *line) {
    unsigned rate;
    char extra;
#if CONFIG_IDF_TARGET_ESP32
    if (esp32_ring_split(line) || esp32_ring_rearm(line) || esp32_ring_pp8(line) ||
        esp32_ring_pingpong(line)) /* RINGPP8 before RINGPP: "RINGPP %u" also matches it */
        return true;
    if (dsp_probe_command(line, burst_serial_send)) return true;
#endif
    if (sscanf(line, "RINGPROBE %u %c", &rate, &extra) != 1) return false;
    if (rate > 6) {
        reply("ERR args\n");
        return true;
    }
    const uint32_t *data;
    unsigned elapsed;
    if (!spectrum_acquire(256, rate, &data, &elapsed)) return true;
#if CONFIG_IDF_TARGET_ESP32
    const uint32_t cr = DUMP_CTRL, wr = DUMP_STATUS, own_reg = DPORT_IRAM_DRAM_AHB_SEL_REG;
    const unsigned count = 16384;
    uint32_t owner = DPORT_REG_READ(own_reg),
             select = (owner & ~DPORT_MAC_DUMP_MODE_M) | (3 << DPORT_MAC_DUMP_MODE_S);
    uint32_t bits = rate == 6 ? BIT(16) : rate == 1 ? BIT(15) : 0;
#elif CONFIG_IDF_TARGET_ESP32S2
    const uint32_t cr = 0x60033d64, wr = 0x60033d68, own_reg = SRAM_OWNER_REG;
    const unsigned count = 8192;
    uint32_t owner = REG_READ(own_reg), select = (owner & ~15u) | 7u;
    uint32_t bits = rate == 6 ? BIT(16) : rate == 1 ? BIT(15) : 0;
#elif CONFIG_IDF_TARGET_ESP32C3
    const uint32_t cr = 0x60033d5c, wr = 0x60033d60, own_reg = SRAM_OWNER_REG;
    const unsigned count = 16384;
    uint32_t owner = REG_READ(own_reg), select = (owner & ~7u) | 2u | 8u;
    uint32_t bits = 0;
#elif CONFIG_IDF_TARGET_ESP32C5
    const uint32_t cr = 0x600a9004, wr = 0x600a9008, own_reg = SRAM_OWNER_REG;
    const unsigned count = 16384;
    uint32_t owner = REG_READ(own_reg), select = (owner & ~0xf00u) | 0x10200u;
    uint32_t bits = 0;
#elif CONFIG_IDF_TARGET_ESP32C6
    const uint32_t cr = 0x600a9004, wr = 0x600a9008, own_reg = SRAM_OWNER_REG;
    const unsigned count = 16384;
    uint32_t owner = REG_READ(own_reg), select = (owner & ~0xf00u) | 0x400u;
    uint32_t bits = 0;
#else
    reply("ERR unsupported\n");
    return true;
#endif
#if !CONFIG_IDF_TARGET_ESP32S31 && !CONFIG_IDF_TARGET_ESP32C61
    volatile uint32_t *probe = (volatile uint32_t *)data;
    for (unsigned j = 0; j < 8; j++) probe[j] = 0xa5c33c5a;
    unsigned irq = portSET_INTERRUPT_MASK_FROM_ISR();
    REG_WRITE(cr, 0);
    REG_WRITE(own_reg, select);
    (void)REG_READ(own_reg);
    uint32_t ctrl = bits | count | BIT(17);
    REG_WRITE(cr, ctrl | BIT(18));
    REG_WRITE(cr, ctrl);
    unsigned start = esp_cpu_get_cycle_count();
    REG_WRITE(cr, ctrl | BIT(31));
    unsigned prev = REG_READ(wr) & 0x1ffffu, first = prev, wraps = 0, changes = 0;
    while (esp_cpu_get_cycle_count() - start < CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ * 1000u) {
        unsigned cur = REG_READ(wr) & 0x1ffffu;
        if (cur != prev) changes++;
        if (cur < prev) wraps++;
        prev = cur;
    }
    unsigned live = 0, freed = 0;
    uint32_t live_words[4];
    for (unsigned j = 0; j < 4; j++) live_words[j] = probe[j];
    for (unsigned j = 0; j < 8; j++)
        if (probe[j] != 0xa5c33c5a) live++;
    REG_WRITE(cr, 0);
    REG_WRITE(own_reg, owner);
    (void)REG_READ(own_reg);
    for (unsigned j = 0; j < 8; j++)
        if (probe[j] != 0xa5c33c5a) freed++;
    portCLEAR_INTERRUPT_MASK_FROM_ISR(irq);
    char text[128];
    snprintf(text, sizeof(text), "RINGPROBE %u %u %u %u %u %u %u\n", rate, first, prev, wraps, changes, live,
             freed);
    reply(text);
    snprintf(text, sizeof(text), "RINGWORDS %08lx %08lx %08lx %08lx / %08lx %08lx %08lx %08lx\n",
             (unsigned long)live_words[0], (unsigned long)live_words[1], (unsigned long)live_words[2],
             (unsigned long)live_words[3], (unsigned long)probe[0], (unsigned long)probe[1],
             (unsigned long)probe[2], (unsigned long)probe[3]);
    reply(text);
#endif
    return true;
}
#endif
