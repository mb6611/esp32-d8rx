# Notice

This repository is a modified version of ESPARGOS esp-sdr
(<https://github.com/ESPARGOS/esp-sdr>), forked at commit
`9b61e7465bee6da15209c5c09308043d08c46a1d`. It is an unofficial fork adding a
profile and host tools for the classic ESP32 DevKit V1. Licensed
GPL-3.0-or-later (see `LICENSE`).

## Apache-2.0 parts (kept under their own license)
- `components/esp-dsp`
- `platform/esp32s2/esp_usb_cdc_rom_console/`
- `main/targets/esp32s3/s3_fft_rnd.S`
- `main/targets/esp32s31/burst/s31_fft_rnd.S`
- ESP-IDF itself (Espressif, Apache-2.0), used as the build toolchain.

## Policy for this fork
- New files carry `SPDX-License-Identifier: GPL-3.0-or-later`.
- Modified upstream files get a `Modified by <author>, 2026` line.
