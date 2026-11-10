# Fresh clone to first spectrum: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## U5.7 fresh clone to first spectrum (SC1)

Method (as U1.6 did): `git clone --recursive` of the local repo into `/tmp/u5-clone`, then copy
the uncommitted working-tree files over it (modified + untracked, minus build dirs and venvs),
then run the commands from `docs/devkitv1/README.md` in order. Wall-clock per step, 2026-11-05:

| Step | Command | Time |
| --- | --- | --- |
| clone + copy | `git clone --recursive …` + copy | < 1 s (local clone) |
| build | `. ~/esp/esp-idf/export.sh; python3 tools/build_firmware.py --profile esp32-devkitv1 --version dev --output /tmp/u5-art` | 49 s (rc 0) |
| host install | `cd host && python3 -m venv .venv && . .venv/bin/activate && pip install -e .[dev]` | 5 s (`pytest`: all passed) |
| flash | `python -m esptool … -b 921600 … write-flash …` (README "Flash") | 11 s |
| info | `esptool` hard reset, then `esp32sdr info` | 2 s (`TRANSPORT UART 921600`) |
| capture | `esp32sdr capture --freq 2437 --rate 40 --n 16380 -o fresh-ch6` | 2 s (CRC ok) |
| spectrum | `esp32sdr spectrum --freq 2437 --bins 1024 -o fresh-spec.csv` | 1 s (44 frames, 128 FFTs) |

Total ≈ 70 s of machine time; with typing, well under the 10 minutes of SC1. **Pass.**
Caveats: the IDF toolchain was already installed and pip packages were cached (a first-time IDF
install is not part of SC1); the local clone hides network time. The working tree at that moment
included uncommitted U6a firmware changes, so this build's `CAPS` also listed `IQS FOFS`.
