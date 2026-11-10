# rtl_tcp bridge check: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## U8 – rtl_tcp bridge scripted on real board (2026-11-05)

`esp32sdr bridge --listen 127.0.0.1:1234 --freq 2402 --dec 512 --bits 8` plus a Python rtl_tcp client (`/tmp/u75/client.py`, now `scripts/client.py`):

- Header `RTL0`, tuner type 5, 29 gains: correct.
- Sustained rate 31 086 / 31 181 S/s over 10 s (expected 31 250).
- Cmd 0x01 → 2400, 2483, 2426 MHz: bridge logged each tune, samples kept flowing after each (rates read 38.7 kS/s only because the client slept 1.5 s and drained the backlog), stream stops reported lost 0 samples.
- Disconnect and reconnect worked (second client got the header and ~31.2 kS/s at the last tuned frequency, 2426).
- After: `esp32sdr info` answers normally; no bugs found, no code changed; pytest 62 passed.
- U8.3 (real gqrx) still needs a human.
