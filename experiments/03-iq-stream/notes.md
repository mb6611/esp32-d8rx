# 60 s I/Q stream: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## U7.5 – 60 s IQS stream at 2402 MHz (2026-11-05)

`esp32sdr stream --freq 2402 --dec 512 --bits 8 --seconds 60 -o /tmp/u75/ble` (now `data/ble.sigmf-*`) on the DevKit V1 (921600 baud):

- 1 874 990 samples (31 250 S/s × 60 s), 3663 frames, bad_crc 0, seq_gaps 0.
- Lost 0 samples (0.0000 %, target < 0.01 %): index gaps 0 + tail 0; firmware IQSEND cross-check reports 0 lost pairs / 512 dec. Loss is counted exactly.
- Plot: `figures/u75-ble-2402.png` (4 ms power vs time + spectrogram).
- Bursts: 39 distinct bursts ≥6 dB above the median in 60 s (e.g. t = 0.08, 1.68, 2.29, 2.63, 2.84, 3.18, 3.40, 3.96 s …). They are irregular, median spacing ≈ 1.07 s (10–90 % range 0.34–2.56 s): several nearby devices advertise at once, so no single fixed interval was resolved. The 31 kHz bandwidth sees only a ~31 kHz slice of a 1 MHz BLE channel, so some packets are missed. Bursts are clearly visible; a single known phone was not isolated.
