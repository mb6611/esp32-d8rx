# Burst robustness: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## U5.6 robustness (SC2, R3, E1)

```
50 × `esp32sdr capture --freq 2437 --rate 40 --n 16380 -o …` (each a new process: open, sync,
tune, capture, release): 0 failures, 84 s total (≈1.7 s per run)

One session, 50 back-to-back CAP16/CAP20 16380 at 2437 MHz each:
80 MS/s  8-bit: 0 failures, 361 ms each
40 MS/s  8-bit: 0 failures, 361 ms each
16 MS/s  8-bit: 0 failures, 362 ms each
80 MS/s 10-bit: 0 failures, 451 ms each
40 MS/s 10-bit: 0 failures, 451 ms each
16 MS/s 10-bit: 0 failures, 452 ms each
INFO afterwards: ESP32SDR 6 burst 16380

$ esp32sdr tune 2600
error: 2600 MHz outside supported range 1850-2500 MHz      (exit 1, nothing sent)
$ esp32sdr tune 1849
error: 1849 MHz outside supported range 1850-2500 MHz      (exit 1)
```

350 captures, zero CRC failures, no resets or hangs. Per-capture time is the UART transfer
(32,760 B at 8-bit ≈ 355 ms at 921600 8N1). **Pass.**
