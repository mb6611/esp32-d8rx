# BLE advertising on/off: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## US2.2 BLE advertising on/off test (2026-11-05)
Stream at 2480 MHz (BLE ch39, clear of Wi-Fi), dec 512, 8-bit, 31.25 kS/s, 20 s per run, 0 lost samples in every run.
iPhone on top of the board running nRF Connect's advertiser (iOS default interval/power), same position for both runs.
Detection: DC removed, |x|^2 smoothed over 3 samples (96 us), threshold = 99.99th percentile of the OFF run; events merged within 2 ms.

| run | events / 20 s | rate | median interval |
|-----|---------------|------|-----------------|
| advertiser off | 27 | 1.35 /s | 400 ms (no periodicity) |
| advertiser on  | 259 | 12.95 /s | 66 ms; histogram peaks at ~35 ms with multiples at 70/105/140 ms |

Result: ~10x more bursts with the advertiser on, periodic at ~35 ms (advertising interval plus BLE's 0-10 ms random delay; multiples are missed detections). US2.2 passes.
Plots: `figures/{compare_on_off,on,off,first_unsure}.png`; script `scripts/analyze.py`. Raw SigMF in `data/`.

### Isolating the phone's packets (timing + strength), same recordings
`scripts/isolate.py` -> `figures/phone_isolation.png`. Pulses = local peaks >= 18 dB above a 50 ms running-median noise floor (gain-independent SNR).
Periodicity search (pairs with gap in [T, T+10.5 ms], BLE's random advDelay) peaks at T = 29.5 ms in the ON run, with harmonics; nothing at that T in the OFF run.
Greedy chain at T: ON -> 515 of 1026 pulses form one train spanning 19.6 s (89% of the expected events; in-train gaps uniformly 29.5-40 ms = T + U(0,10 ms)); OFF -> only 24 pulses chain by chance (4%).
Train pulses are stronger (median SNR 23.3 dB vs 20.7 dB for other pulses), consistent with the phone lying on the board.
The OFF run shows a different weak periodic source near 90 ms (peaks at ~180/270 ms), i.e. another nearby device.
