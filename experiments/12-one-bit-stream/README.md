# 12 · 1-bit samples to fit FSK through the UART

The first decode in [11](../11-d8-decoding/README.md) worked, but only just. The widest stream the 921600-baud link allowed was 4-bit samples at 62.5 kS/s, which gives 2 samples per D8 bit and folds the two FSK tones onto each other. I wanted enough samples per bit to decode cleanly, through the same link.

I was stuck on this until I thought about it in terms of information. A D8 packet carries about 31 kbit/s, and the UART carries about 737 kbit/s, more than twenty times that. So the link was never too slow; I was spending its bits badly. Plain I/Q samples spend most of their bits on amplitude, and FSK is constant-envelope: the transmitter never changes its amplitude, and all the information is in which way the phase turns. So I added a 1-bit mode to the firmware. After the filter it keeps only the sign of I and the sign of Q and packs four complex samples into one byte. That loses the amplitude, which wasn't needed, and keeps the phase to within a quarter turn, which is enough to see which way it spins.

With 1 bit, 250 kS/s (decimation 64) is 62.5 KB/s, 69.7% of the link, and gives 8 samples per D8 bit. I first wanted 125 kS/s to leave more room, and in simulation it failed. Taking signs is very nonlinear: a hard-limited tone comes back with odd harmonics at 3, 5 and 7 times its frequency. At 125 kS/s the −3rd harmonic of one tone, at −95 kHz, wraps around to +30 kHz and lands on the other tone. At 250 kS/s it stays out of the way. The simulated bit error rate went from 1–23% at 125 kS/s to about 0–0.2% at 250 kS/s.

Then the firmware fell behind. Core 0 runs the filter, and at 250 kS/s output that is about 24 million multiply-adds a second. GCC's version of the dot product loaded one 16-bit value at a time and took about 220 cycles per input block, around 92% of the core, and the queue in front of it overflowed. The ESP32's Xtensa core has a multiply-accumulate unit that can multiply the low and the high halves of two 32-bit registers, so I wrote the inner loop in inline assembly: two 32-bit loads feed two multiply-adds, in a hardware loop with no branch overhead. I also handed the filter whole runs of blocks instead of one at a time. That brought it to 77–119 cycles per block, about 50% of core 0.

On the bench, tuned to ambient traffic at 2440 MHz, I ran three 20 s streams at 1-bit, decimation 64, plus a 20 s recording through the normal `esp32sdr stream` command. All four had 0 index gaps, 0 dropped frames and 0 lost samples. The other core's worst-case work per block was 99,886–107,620 cycles of a 120,960-cycle budget, and the block queue peaked at about 1,100 of 8,192 entries. At decimation 128, core 0 was 43% busy. I also made a short 8-bit check run to make sure the ordinary widths still worked.

This experiment only shows that the stream is clean; it doesn't decode anything. The D8 decoding with 1-bit samples is in 11: 22 against 3 CRC-valid packets for mode 2 against mode 0, then 47 of 47 once the frequency and gain were right. All later D8 work uses this mode.

There are no figures for this experiment.

---

## Numbers

| What | Value | Where |
|---|---|---|
| 1-bit, dec 64 | 250 kS/s, 8 samples per bit, 69.7% of the link | [docs/devkitv1/iq-stream-uart.md](../../docs/devkitv1/iq-stream-uart.md) |
| Simulated bit error rate, 125 vs 250 kS/s | 1–23% vs about 0–0.2% | same |
| Filter cost on core 0, before / after | about 220 / 77–119 cycles per block | [docs/lab-log](../../docs/lab-log/README.md) |
| Bench, three 20 s runs + one 20 s `esp32sdr stream` | 0 gaps, 0 frame drops, 0 lost samples; core 0 about 50% | docs/devkitv1/iq-stream-uart.md |
| `iqs1_cli` recording | 4,999,932 samples at 250 kS/s (20.0 s), 2440 MHz | `data/iqs1_cli.sigmf-meta` |

## Files

- Notes: the "1-bit (hard-limited) samples" section of [docs/devkitv1/iq-stream-uart.md](../../docs/devkitv1/iq-stream-uart.md) (kept there because it is the link-budget reference).
- `data/`: `iqs1_d64.npy` (16:17), `iqs1_cli.sigmf-*` (16:18, 20 s via `esp32sdr stream`), `iqs_reg.sigmf-*` (16:19, 8-bit check run, 10 s at 31.25 kS/s). See [data/DATA.md](data/DATA.md); all three recordings are in the release asset (`experiments/fetch-data.sh`).
- `scripts/bench_iqs1bit.py`: the bench check (needs the board).
- Not in this folder: the bit-error simulation and the console output of the three bench runs; their numbers are in the notes above.

## How to re-run

With the board connected, run `host/.venv/bin/python experiments/12-one-bit-stream/scripts/bench_iqs1bit.py --dec 64 --seconds 20 --freq 2440`.

## Ran

2026-11-05 15:57–16:22 EST (start 15:57, 1-bit builds and recordings 16:16–16:19). Commits 81a6e74, 627359d (17:26).

**Overlap:** Ran in parallel with [10](../10-fast-retune/README.md) and [11](../11-d8-decoding/README.md).

[← 11-d8-decoding](../11-d8-decoding/README.md) · [index](../README.md) · [13-d8live-one-channel →](../13-d8live-one-channel/README.md)
