/* SPDX-License-Identifier: GPL-3.0-or-later
 * ESP32 DevKit V1 fork: fast-retune (frequency hop) feasibility probe.
 * Built only with -DHOP_PROBE=ON; never packaged. Included by receiver.c after
 * tune_rx() and burst_capture.h. Results: experiments/10-fast-retune/notes.md.
 *
 * A fast retune is the synthesizer part of tune_rx() only, never the DC/gain
 * recalibration. Frequencies are absolute kHz. Modes:
 *   0  set_chanfreq(2412) + capacitor-code release + rom_set_rf_freq_offset (as tune_rx)
 *   1  capacitor-code release + rom_set_rf_freq_offset
 *   2  rom_set_rf_freq_offset only
 *
 *   HOPFULL khz              ordinary tune with recalibration at khz (FREQ + FOFS path)
 *   HOPT ka kb n mode        n fast retunes alternating ka/kb; per-phase CCOUNT timing
 *     reply: HOPT n mode chan=<med>/<max> cap=.. rom=.. total=..  (units: 0.01 us)
 *   HOPCAP khz mode rate n at_us fmt
 *                            capture n samples (rate 0/1/6, fmt 16/20); fast retune to khz
 *                            at_us after the capture starts (at_us 0: retune first, then
 *                            capture). khz 0: no retune.
 *     reply: HOPCAP t0=<0.01us> t1=<0.01us> stale=<0|1> then the CAP DATA block
 *            (t0/t1 = retune start/end relative to capture start; negative if before)
 *   HOPREG                   I2C block 0x62 host 1 regs 0..15 (for a lock bit)
 */
#include "esp_cpu.h"

#define HOP_MAX 512
static unsigned hop_xtal_code(void) {
    unsigned xtal = rtc_clk_xtal_freq_get();
    return xtal == 26 ? 1 : xtal == 24 ? 2 : 0;
}

typedef struct {
    uint32_t chan, cap, rom;
} hop_cycles_t;

static void hop_retune(unsigned khz, unsigned mode, unsigned xtal, hop_cycles_t *t) {
    unsigned mhz = khz / 1000u;
    int offset = (int)(((khz % 1000u) * 1024u + 500u) / 1000u);
    uint32_t a = esp_cpu_get_cycle_count();
    if (mode == 0) set_chanfreq(2412, 0);
    uint32_t b = esp_cpu_get_cycle_count();
    if (mode != 2) {
        unsigned cap = ram_chip_i2c_readReg(0x62, 1, 0);
        ram_chip_i2c_writeReg(0x62, 1, 0, cap & ~0x80u);
    }
    uint32_t c = esp_cpu_get_cycle_count();
    rom_set_rf_freq_offset(xtal, mhz, offset);
    uint32_t d = esp_cpu_get_cycle_count();
    if (t) *t = (hop_cycles_t){b - a, c - b, d - c};
}

static int hop_cmp(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

/* Median and max of n cycle counts, in 0.01 us. */
static void hop_stats(uint32_t *v, unsigned n, unsigned *med, unsigned *max) {
    qsort(v, n, sizeof(*v), hop_cmp);
    const unsigned f = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
    *med = v[n / 2] * 100u / f;
    *max = v[n - 1] * 100u / f;
}

static bool hop_valid_khz(unsigned khz) {
    return khz >= 2300000u && khz <= 2560000u;
}

static int hop_cc_to_cus(uint32_t from, uint32_t to) {
    return (int32_t)(to - from) * 100 / (int)CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
}

static bool hop_capture(unsigned khz, unsigned mode, unsigned rate, unsigned n, unsigned at_us,
                        unsigned format) {
    if (n < 256 || n > MAX_SAMPLES || (rate != 0 && rate != 1 && rate != 6) || mode > 2 ||
        (format != 16 && format != 20) || (khz && !hop_valid_khz(khz)) || at_us > 5000) {
        reply("ERR args\n");
        return false;
    }
    unsigned clock = rate == 0 ? 1 : rate == 1 ? 2 : 0;
    unsigned xtal = hop_xtal_code();
    REG_WRITE(DUMP_CTRL, 0);
    for (unsigned j = 0; j < CAPACITY; j++) samples[j] = SENTINEL;
    filter_apply();
    uint32_t owner = DPORT_REG_READ(DPORT_IRAM_DRAM_AHB_SEL_REG);
    uint32_t byte_select = REG_READ(DUMP_BYTES);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG,
                    (owner & ~DPORT_MAC_DUMP_MODE_M) | (3 << DPORT_MAC_DUMP_MODE_S));
    REG_WRITE(DUMP_BYTES, 0x03020100);
    uint32_t ctrl = n | ((clock == 0) << 16) | ((clock == 2) << 15);
    uint32_t r0 = 0, r1 = 0;
    if (khz && at_us == 0) {
        r0 = esp_cpu_get_cycle_count();
        hop_retune(khz, mode, xtal, NULL);
        r1 = esp_cpu_get_cycle_count();
    }
    REG_WRITE(DUMP_CTRL, ctrl | BIT(31));
    int64_t start = esp_timer_get_time();
    REG_SET_BIT(DUMP_CTRL, BIT(19));
    uint32_t s = esp_cpu_get_cycle_count();
    REG_CLR_BIT(DUMP_CTRL, BIT(19));
    if (khz && at_us) {
        while (esp_cpu_get_cycle_count() - s < at_us * CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ) {}
        r0 = esp_cpu_get_cycle_count();
        hop_retune(khz, mode, xtal, NULL);
        r1 = esp_cpu_get_cycle_count();
    }
    while (!(REG_READ(DUMP_CTRL) & BIT(18)) && esp_timer_get_time() - start < 20000) {}
    uint32_t result = REG_READ(DUMP_CTRL), status = REG_READ(DUMP_STATUS);
    unsigned elapsed = esp_timer_get_time() - start;
    REG_WRITE(DUMP_CTRL, 0);
    esp_rom_delay_us(10);
    DPORT_REG_WRITE(DPORT_IRAM_DRAM_AHB_SEL_REG, owner);
    REG_WRITE(DUMP_BYTES, byte_select);
    filter_restore();
    if (!(result & BIT(18)) || (status & 0x7fff) != n) {
        reply("ERR capture_timeout\n");
        return false;
    }
    for (unsigned j = 0; j < CAPACITY; j++)
        if ((j < n && samples[j] == SENTINEL) || (j >= n && samples[j] != SENTINEL)) {
            reply("ERR capture_memory %u\n", j);
            return false;
        }
    reply("HOPCAP t0=%d t1=%d stale=%u\n", khz ? hop_cc_to_cus(s, r0) : 0, khz ? hop_cc_to_cus(s, r1) : 0,
          (unsigned)rx_recalibration_stale());
    return burst_send_capture(n, format, elapsed);
}

static bool hop_probe_command(const char *line) {
    unsigned a, b, n, mode, rate, at, fmt;
    char extra;
    if (sscanf(line, "HOPFULL %u %c", &a, &extra) == 1) {
        if (!hop_valid_khz(a)) {
            reply("ERR args\n");
            return true;
        }
        frequency_mhz = a / 1000u;
        esp32_fofs = (int)(a % 1000u);
        rx_recalibrate(frequency_mhz);
        rx_ready = false;
        prepare_rx();
        reply("OK\n");
        return true;
    }
    if (sscanf(line, "HOPT %u %u %u %u %c", &a, &b, &n, &mode, &extra) == 4) {
        if (!hop_valid_khz(a) || !hop_valid_khz(b) || n < 1 || n > HOP_MAX || mode > 2) {
            reply("ERR args\n");
            return true;
        }
        static uint32_t ch[HOP_MAX], cp[HOP_MAX], rm[HOP_MAX], tot[HOP_MAX];
        unsigned xtal = hop_xtal_code();
        for (unsigned j = 0; j < n; j++) {
            hop_cycles_t t;
            hop_retune(j & 1 ? b : a, mode, xtal, &t);
            ch[j] = t.chan;
            cp[j] = t.cap;
            rm[j] = t.rom;
            tot[j] = t.chan + t.cap + t.rom;
        }
        unsigned m[4], x[4];
        hop_stats(ch, n, &m[0], &x[0]);
        hop_stats(cp, n, &m[1], &x[1]);
        hop_stats(rm, n, &m[2], &x[2]);
        hop_stats(tot, n, &m[3], &x[3]);
        reply("HOPT %u %u chan=%u/%u cap=%u/%u rom=%u/%u total=%u/%u stale=%u\n", n, mode, m[0], x[0], m[1], x[1],
              m[2], x[2], m[3], x[3], (unsigned)rx_recalibration_stale());
        return true;
    }
    if (sscanf(line, "HOPCAP %u %u %u %u %u %u %c", &a, &mode, &rate, &n, &at, &fmt, &extra) == 6) {
        hop_capture(a, mode, rate, n, at, fmt);
        return true;
    }
    if (!strcmp(line, "HOPREG")) {
        char text[200];
        int len = snprintf(text, sizeof(text), "HOPREG");
        for (unsigned r = 0; r < 16; r++)
            len += snprintf(text + len, sizeof(text) - len, " %02x", ram_chip_i2c_readReg(0x62, 1, r) & 0xffu);
        reply("%s\n", text);
        return true;
    }
    return false;
}
