# 10 · Can the board retune fast enough to hop?

Listening to one channel means hearing one D8 packet in 47. To hear them all, the board has to hop along with the E8. A slot is about 9 ms and a packet about 6.7 ms, so there are roughly 2 ms between packets to retune. A normal tune on the ESP32 takes 25–60 ms, because it recalibrates the receiver every time. So I wanted to know how long the part that actually moves the oscillator takes, and whether the receiver still works if I skip the calibration.

I built a probe firmware (`-DHOP_PROBE=ON`) that times each piece of a tune with the CPU cycle counter and can retune in the middle of a capture. A normal tune does three things after calibrating: `set_chanfreq`, a release of a capacitor code (bit 7 of register 0 in I2C block 0x62, host 1), and the ROM call `rom_set_rf_freq_offset` that moves the synthesizer. I calibrated once at 2440 MHz and then timed 200 retunes between 2404 and 2477 MHz for each combination. All three pieces took 421 µs median and 500 µs at most. The capacitor release took 12 µs and the ROM call 245 µs, so those two alone (mode 1) took 257 µs median, 265 µs at most. `set_chanfreq` turned out not to be needed, and it also makes a 20 µs saturating burst about 240 µs into the call. The ROM call alone (mode 2) works only while that capacitor bit stays clear, so mode 1, with its 12 µs release, is the one to use.

A retune is only finished when the oscillator is on the new frequency, so I needed something to watch. The board's 40 MHz crystal leaks harmonics into its own receiver, and at 2400.000 and 2480.000 MHz they are steady narrow lines. A line sits at the right offset only if the oscillator is where I asked it to be, so it works as a ruler. I started a fast retune 100 µs into a 1 ms capture at 16 MS/s, 20 times per direction at manual gain 50, and measured each line's power in 8 µs windows. The old line was gone 19–72 µs after the call started, depending on the mode, and the new line was within 3 dB of its final level 2–63 µs before the call returned. In other words, there is nothing to wait for after the call: the receiver is usable when it returns. Across the different pairs and directions the ROM call itself took 185–265 µs.

Skipping calibration is only safe if one calibration holds over the whole band. Calibration exists for Wi-Fi, where 64-QAM needs DC offset and I/Q imbalance removed carefully on weak signals; FSK from a transmitter a metre away only needs the phase direction. To check, I calibrated once at 2404 MHz, the band edge and so the worst case, then fast-retuned through all 47 D8 channels in hop order and compared each with a fully recalibrated tune at the same frequency (80 MS/s, manual gain 50, 4–6 captures of 16,380 samples per channel and method). On every channel the ruler line landed in the expected FFT bin; the largest position error was 4.1 kHz, under one 4.88 kHz bin. The median line SNR was 23.9 dB both ways, the quiet-window DC 3.1 against 2.9, and the clipping followed the ambient Wi-Fi bursts rather than the tune method. The three weakest lines (5–9 dB, near 2440 MHz, where the line is only 1.8–3.9 MHz from the centre) were just as weak after a full recalibration. An earlier sweep calibrated at 2440 MHz, at 40 MS/s, agreed.

I also looked for a lock bit and didn't find one: across a retune only register 5 of that block changed (0x7c to 0x66), which looks like the band code the ROM call picks. The crystal lines stay the practical lock check.

So a hop costs about 0.26 ms out of the 2 ms gap, with one calibration at boot. What this didn't cover: the hardware gain control across the band (the sweep used manual gain), temperature drift, and retuning while a stream is running. The last two were measured later in [14](../14-hop-follow/README.md): retuning mid-stream lost no samples, and a 10.5-minute run with no recalibration in the hop path never lost lock.

There are no figures for this experiment; the result tables are in [notes.md](notes.md).

---

## Numbers

| What | Value | Where |
|---|---|---|
| Retune, mode 1 (cap release + ROM call), 2404 ↔ 2477 | 257 µs median, 265 µs max | `notes.md` (retune time) |
| Retune, mode 0 (with `set_chanfreq`) | 421 µs median, 500 µs max | `notes.md` |
| New line within 3 dB of final | 2–63 µs before the call returns | `data/settle-16msps.txt` |
| Ruler line position error, 47 channels, calibrated once | 4.1 kHz max, 1.2 kHz median (bin 4.88 kHz) | `data/sweep-ref2404-80msps.log` |
| Line SNR min / median, fast vs recalibrated | 5.4 / 23.9 dB vs 2.3 / 23.9 dB | `data/sweep-ref2404-80msps.log` |

## Files

- [notes.md](notes.md): the full original notes (probe commands, settle table, sweep table).
- `data/`: `settle-16msps.txt` and `sweep-*.log` (the result tables), plus the raw captures behind them (`s_*-q.npz`, `settle*.npz`, `hop_try.npz`, `hop_sweep_*.npz/.log`). See [data/DATA.md](data/DATA.md).
- `scripts/probe_hop.py`: the probe host side (needs the board and a `-DHOP_PROBE=ON` build; probe code in `main/targets/esp32/hop_probe.h`). It imports `probe_ring.py` from [01](../01-ring-feasibility/README.md).
- `scripts/settle_an.py` (settle table), `scripts/sweep_sum.py` (sweep summary): offline, recovered from `/tmp`.
- Not kept: the raw captures of the first part of the 80 MS/s sweep (`part1`); only its log survives.

## How to re-run

Fetch the release data first: `experiments/fetch-data.sh`.

```sh
D=experiments/10-fast-retune/data; S=experiments/10-fast-retune/scripts
host/.venv/bin/python $S/settle_an.py $D/s_big_m0-q.npz $D/s_big_m1-q.npz $D/s_big_m2-q.npz $D/s_small_m1-q.npz   # = data/settle-16msps.txt
host/.venv/bin/python $S/sweep_sum.py $D/hop_sweep_ref2404_80_part2.npz
```

## Ran

2026-11-05 15:40–16:11 EST (start 15:40, `hop_try.npz` 15:47, sweeps 15:56–16:10, settle captures 15:57–16:01, scripts and notes 16:11). Commit 629d4c6 (17:26).

**Overlap:** Ran in parallel with [11](../11-d8-decoding/README.md) (same start, first decode and mode 2 against mode 0) and [12](../12-one-bit-stream/README.md); they shared the board, one at a time.

[← 09-d8-hop-cycle](../09-d8-hop-cycle/README.md) · [index](../README.md) · [11-d8-decoding →](../11-d8-decoding/README.md)
