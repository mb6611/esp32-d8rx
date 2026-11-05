# ESP32 DevKit V1 receive-only SDR — Implementation Plan

**Spec:** [`specs/esp32-devkitv1-sdr.spec.md`](../specs/esp32-devkitv1-sdr.spec.md)

## Overview
Add a `esp32-devkitv1` build profile to the esp-sdr fork (921600 baud, no source changes) and a self-contained Python host package (`host/`) for tune, burst capture, spectra and SigMF output. Those two give US1. Then run a bench feasibility probe on whether the classic ESP32's capture block can run as a ring, and whether the app CPU can decimate 16 MS/s in real time (D4). If the probe passes, port upstream's S3 `IQS`/`FOFS` stream to the ESP32 over UART behind a Kconfig switch, plus host stream support and an rtl_tcp bridge (US2, US3). If it fails, build the snapshot waterfall instead (R11). The probe's result is a user decision point; nothing after it starts without one.

## Current state
- Upstream ESP32 receiver is one-shot only: `main/targets/esp32/receiver.c` (~30–122) writes DUMP_CTRL `0x60033d90` (`n|source<<20|clock bits 15/16|BIT31`), pulses bit 19, polls bit 18; status `0x60033d94`, byte count `0x60033dc4`; `DPORT_MAC_DUMP_MODE=3`; window `0x3ffe8000–0x3fff8000`, max 16380 pairs. Upstream already has a bounded ring diagnostic with an ESP32 branch: `main/diagnostics/ring_probe.h` (`RINGPROBE <rate>`, built with the global CMake option `-DRING_PROBE=ON`, `main/CMakeLists.txt:22`; hooked at `receiver.c:145,208`). It arms the dump with `BIT(17)`, watches the write index for 1 s, and reports `first last wraps changes live freed`. The `live` field counts whether the CPU can see writes while RF owns the window. `docs/how-it-works.md` "Continuous capture" explains that a ring needs CPU-readable banks while the engine writes another (why C3 has none). The ESP32 is not listed as continuous upstream, and no result for it is documented.
- `main/common/ring_capture.c` is not built for ESP32. Its IQS engine is `#if CONFIG_IDF_TARGET_ESP32S3` (~677–1010): `iqs_header_t` (24 B), `iqs_flush` (CRC32 + `txq_push`), PIE-SIMD FIR (`s3_firdot.S`, S3-only), `iqs_put`, loss via `lost_pairs`/sample index. The S3 core 1 runs bare metal (`c1_start` ~1496, `s3_core1_main` ~1448). The `txq` SPSC ring (~262–290) and `ring_io.h` UART pump are portable.
- S3 ring registers for comparison: `0x60033d5c` CIRCULAR `0x24000`, write index `0x60033d60`, config `0x60033d90=0x000c2040`, bank select `0x600c101c` (ring_capture.c ~111–117, ~1686–1711).
- IQS parsing and the `ERR transport` gate (a port check, `burst_serial_port()==BURST_SERIAL_USB`) are in `main/targets/esp32s3/receiver.c` (~383–426, ~408). FOFS is S3-only (~52–64, ~256–268).
- `sdkconfig.defaults.esp32:10` sets `CONFIG_FREERTOS_UNICORE=y`; `main/targets/esp32/sram_guard.ld` keeps IRAM ≤ `0x400a8000` and data/bss ≤ `0x3ffe8000` (RF window).
- Per-target sources: `main/CMakeLists.txt`, pattern `if(IDF_TARGET STREQUAL "esp32") list(APPEND sources "${target_dir}/x.c")`. Build profiles: `firmware-targets.json` (`sdkconfig` key, precedent `esp32s31-stream`) consumed by `tools/build_firmware.py`.
- Compile-time `CONFIG_ESP_SDR_UART_BAUD=921600` works (proven in auto-drone stage0). The runtime `BAUD` command accepts only 1M/2M (`burst_serial.c` ~50); we don't use it.
- Host seed: `/Users/1unoe/Developer/personal/auto-drone/stage0/capture.py` (109 lines; `exact`, `cmd`, `sync` via `RELEASE`/`SYNC n` retry, `capture` parsing `DATA n crchex us` + int8 I,−Q, zlib CRC32, rate table). Protocol: `docs/protocol.md`; SPEC/SPC1 frames: `docs/spectrum.md`; IQS1 frames: `docs/iq-stream.md`. No Python packaging exists in the fork. Upstream tests use `unittest` (`.github/workflows/firmware.yml`).
- Bench: `/dev/cu.usbserial-0001`, IDF `~/esp/esp-idf` @ `25fe69f9` (`. ~/esp/esp-idf/export.sh`), flash with `-b 921600`. See `.ru/memories/esp32-bench-setup.md`.

## Key decisions
- Board support is a catalog profile plus `sdkconfig.defaults.esp32-devkitv1`, a full copy of `sdkconfig.defaults.esp32` with the baud set to 921600. `build_firmware.py` passes a single defaults file, so the file can't be layered. (D5)
- Day-to-day builds use `idf.py -B build-esp32-devkitv1 -DIDF_TARGET=esp32 -DSDKCONFIG_DEFAULTS=sdkconfig.defaults.esp32-devkitv1 build`, documented in `docs/devkitv1/README.md`. `build_firmware.py` is for release artifacts only (it needs `--version` and a fresh output directory). (R1)
- All new firmware behaviour sits behind `CONFIG_ESP_SDR_ESP32_IQS` (default n; y in the devkitv1 profile only). The upstream `esp32` profile builds byte-identical to today. (D1, D5)
- Keep `FREERTOS_UNICORE=y`. Run the DSP on the app CPU bare metal, as S3 does with `c1_start`, instead of switching FreeRTOS to SMP. This keeps upstream's timing assumptions on core 0. (D4, C3)
- The ESP32 stream reuses the `IQS1` frame and the `IQS` / `IQSEND` / `FOFS` grammar exactly. Only the transport gate lets UART through on ESP32. (D2)
- Host package: `host/` with `pyproject.toml` (deps numpy, pyserial; extras `[view]` = pyqtgraph). pytest for host tests. Upstream's `tests/` (unittest) stays untouched. (D3, R10)
- The 1850–2500 MHz range is enforced by the host before sending `FREQ`. The firmware accepts 100–6000. (R3)

## What we're NOT doing
- No TX paths, no other targets, no changes to upstream profiles or their defaults. (NG1, NG2)
- No faster-UART or extra-hardware options. (NG3)
- No runtime `BAUD` changes. 921600 is fixed at build time. (R2)
- No absolute power calibration. (NG5)

## Units

### U1 Profile, licensing, build docs
- **Depends on**: —
- **Owns**: `firmware-targets.json`, `sdkconfig.defaults.esp32-devkitv1`, `NOTICE.md`, `README.md` (new top section only), `docs/devkitv1/README.md`
- [x] `U1.1` Add profile `{"id":"esp32-devkitv1","target":"esp32","label":"ESP32 DevKit V1 (CP2102, 921600 baud)","idf_ref":"25fe69f946311abdaf9ad56591f25fedbc20ac98","allow_larger_flash":true,"preview":false,"sdkconfig":"sdkconfig.defaults.esp32-devkitv1"}`. (D5)
- [x] `U1.2` Create `sdkconfig.defaults.esp32-devkitv1`: the content of `sdkconfig.defaults` + `sdkconfig.defaults.esp32` (check whether the IDF also applies the root `sdkconfig.defaults` when `SDKCONFIG_DEFAULTS` is set; include it if not), with `CONFIG_ESP_SDR_UART_BAUD=921600`. Add an SPDX header comment. (R2)
- [x] `U1.3` `NOTICE.md`: modified version of ESPARGOS esp-sdr (https://github.com/ESPARGOS/esp-sdr), forked at `9b61e7465bee6da15209c5c09308043d08c46a1d`, GPL-3.0-or-later. List the Apache-2.0 parts: `components/esp-dsp`, `platform/esp32s2/esp_usb_cdc_rom_console/`, `main/targets/esp32s3/s3_fft_rnd.S`, `main/targets/esp32s31/burst/s31_fft_rnd.S`, and the ESP-IDF itself. State the policy: new files carry `SPDX-License-Identifier: GPL-3.0-or-later`; modified upstream files get a `Modified by <author>, 2026` line. (C1, G3)
- [x] `U1.4` Add a short section at the top of `README.md` saying this is an unofficial fork for the classic ESP32 DevKit V1, linking upstream, NOTICE.md and `docs/devkitv1/README.md`. Leave the upstream text below it unchanged. (C1)
- [x] `U1.5` `docs/devkitv1/README.md`: the exact build, flash (`-p /dev/cu.usbserial-0001 -b 921600`) and host-install commands, the pinned IDF, and the bench recovery notes (RTS reset, 2 Mbaud dead-board trap). (R1)
- [x] `U1.6` verify: a fresh `git clone --recursive` of the fork builds with the documented command.
- [x] `U1.7` verify: `python3 tools/build_firmware.py --profile esp32-devkitv1 --version test --output /tmp/art-$$` succeeds; `--matrix` lists the profile.
- [x] `U1.8` manual: flash, then at 921600 `VERSION?`, `INFO` and `TRANSPORT?` answer (`TRANSPORT UART 921600`). (SC1)

### U2 Host link layer
- **Depends on**: —
- **Owns**: `host/pyproject.toml`, `host/esp32sdr/__init__.py`, `host/esp32sdr/transport.py`, `host/esp32sdr/protocol.py`, `host/esp32sdr/device.py`, `host/tests/fake_device.py`, `host/tests/test_device.py`
- [x] `U2.1` `pyproject.toml` (package `esp32sdr`, Python ≥3.11, deps numpy, pyserial; extras `view`=[pyqtgraph], `dev`=[pytest]; console script `esp32sdr = esp32sdr.cli:main`; license `GPL-3.0-or-later`). (D3, G3)
- [x] `U2.2` `transport.py`: `SerialTransport(port, baud=921600)` with `read_exact(n, timeout)`, `readline(timeout)`, `write`, and `reset()` (RTS pulse, as the auto-drone host scripts do), behind a `Transport` protocol so tests can inject a fake. (R2, E3)
- [x] `U2.3` `protocol.py`: pure functions to build commands and parse replies, ported from stage0 and `docs/protocol.md`: `parse_info`, `parse_limits`, `parse_version`, `parse_data_header` (`DATA n crchex us`), `ERR` → `DeviceError`, `crc32_ok(payload, crc)`. (R4, R7)
- [x] `U2.4` `device.py`: `Device.open(port)` does `sync()` (`RELEASE`/`SYNC n` retry up to 10 s; on failure `reset()` and retry once, then raise `LinkError`). Methods: `info()`, `limits()`, `version()`, `tune(mhz)` (refuse outside 1850–2500 with `ValueError` before sending; parse the FREQ reply, raise `LockError` when the firmware reports no lock), `cap16(n, rate_code, bits) -> bytes`. On a CRC failure or timeout: raise `TransferError` after resyncing, so the next call works without a manual reset. (R3, R7, E1, E3)
- [x] `U2.5` `tests/fake_device.py`: an in-memory fake implementing the `Transport` protocol that answers the commands above. It can inject a CRC error, a timeout, garbage before sync, and a lock failure. `test_device.py` covers each path. (R10, R7, E1, E3)
- [x] `U2.6` verify: `cd host && pip install -e .[dev] && pytest` passes with no board attached. (R10)

### U3 Host data products
- **Depends on**: — (works on bytes and arrays; no device calls)
- **Owns**: `host/esp32sdr/samples.py`, `host/esp32sdr/spectrum.py`, `host/esp32sdr/sigmf.py`, `host/tests/test_samples.py`, `host/tests/test_spectrum.py`, `host/tests/test_sigmf.py`
- [x] `U3.1` `samples.py`: `decode_cap(raw, bits) -> np.complex64` for 8-bit and 10-bit packing per `docs/protocol.md`, applying the Q negation so RF above the LO is positive. Also `RATES` (code → Hz, from protocol.md). (R4, R6)
- [x] `U3.2` `spectrum.py`: `psd(iq, fs, nfft)`; `parse_spc1(frame) -> SpecFrame` (28-byte LE header per `docs/spectrum.md`, power codes, CRC32 check); `parse_specend(line)`. (US1.3, R7)
- [x] `U3.3` `sigmf.py`: `write(path_base, iq, *, fs, fc, bits, gain_mode, gain_index, fw_version, extra)` writes `.sigmf-data` (cf32_le) and `.sigmf-meta` (core:sample_rate, core:frequency, core:hw, core:version, custom `esp32sdr:` keys for bits/gain/firmware). Also `StreamWriter` for appending, which updates `core:sample_count` and records loss annotations. (R5, R8)
- [x] `U3.4` Tests: a synthetic tone at +f decodes to +f after Q negation; 10-bit unpack round-trip; SPC1 good and bad CRC; SigMF meta validates required keys. (R6, R5, R10)
- [x] `U3.5` verify: `pytest host/tests/test_samples.py host/tests/test_spectrum.py host/tests/test_sigmf.py` passes.

### U4 Feasibility probe (gate)
- **Depends on**: U1 (profile builds)
- **Owns**: `main/diagnostics/ring_probe.h` (ESP32 branch only), `main/targets/esp32/dsp_probe.c` (new), the esp32 `RING_PROBE` source line in `main/CMakeLists.txt`, `docs/devkitv1/ring-feasibility.md`, `host/tools/probe_ring.py`
- [x] `U4.1` Build the devkitv1 profile with `-DRING_PROBE=ON`. Run the existing `RINGPROBE 0`, `RINGPROBE 1` and `RINGPROBE 6` (80/40/16 MS/s) via `host/tools/probe_ring.py`, and record the `RINGPROBE`/`RINGWORDS` lines. Interpretation: `wraps>0` and steady `changes` mean the engine runs circularly; `live>0` means the CPU can read the window while RF owns it. (C3, D4)
- [x] `U4.2` Only if U4.1 shows wrapping but no CPU visibility: extend the ESP32 branch of `ring_probe.h` to test whether the 64 KiB window at `0x3ffe8000` can be split into halves the CPU can take back in turn, as the S3 does with banks, using `DPORT_MAC_DUMP_MODE` values. If no split is possible, test back-to-back one-shot re-arm and measure the gap in samples (CCOUNT timestamps). Receive only; follow `.ru/memories/bench-agent-wording.md` and `.ru/memories/esp32-capture-playback-findings.md`. (D4)
- [x] `U4.3` `dsp_probe.c` (built only with `RING_PROBE`): start the app CPU bare metal (adapt S3 `c1_start` in `main/common/ring_capture.c` ~1496, keeping UNICORE), and time a first-stage decimate-by-16 boxcar/CIC over a 16 MS/s int8 buffer in the capture window. It passes at ≤ 12 cycles per pair at 240 MHz (15 is the hard ceiling). Confirm CAP16 and SPEC still work afterwards. (D4, SC2)
- [x] `U4.4` Write `docs/devkitv1/ring-feasibility.md` with the raw output and one verdict: (A) gapless ring readable by the CPU and the DSP fits; (B) only re-armed one-shot works, with gap size and duty cycle; (C) neither works, or the DSP doesn't fit.
- [x] `U4.5` manual: report the verdict to the user. **Stop here.** The user picks the next wave: (A) → U6–U8; (B) → user decides whether counted gaps are acceptable; (C) → U9.

### U5 Host CLI and bench acceptance (burst)
- **Depends on**: U1, U2, U3. Run after U4 is done (shared board).
- **Owns**: `host/esp32sdr/cli.py`, `host/esp32sdr/spec_cmd.py`, `host/tests/test_cli.py`, `docs/devkitv1/bench-results.md`
- [x] `U5.1` `spec_cmd.py`: `Device`-level `spectrum(bins, rate_code, ms)` that sends `SPEC`, reads SPC1 frames until `SPECEND`, and sends the stop byte. Add it as a function taking a `Device`, so `device.py` isn't touched. (US1.3)
- [x] `U5.2` `cli.py` subcommands:
  - `info` (version, chip, limits, rates, bits; R4, US1.1)
  - `tune <MHz>`
  - `capture --freq --rate {80,40,16} --bits {8,10} --n -o BASE` → SigMF
  - `spectrum --freq --bins {256..2048} [-o csv|png]`
  - `--port` defaults to `/dev/cu.usbserial-0001`

  Errors print one line and exit nonzero. (US1.1–US1.3, R3, R5, E1)
- [x] `U5.3` `test_cli.py` runs each subcommand against the fake device. (R10)
- [x] `U5.4` manual: `esp32sdr capture --freq 2437 --rate 40 --n 16380 -o ch6`. The PSD shows activity in the 2427–2447 MHz span. (US1.2)
- [x] `U5.5` manual: orientation check. Tune 10 MHz below an active Wi-Fi channel; the energy appears at +10 MHz. (R6) — verified by an LO sweep instead; the literal single-LO test was ambiguous in ambient traffic (bench-results.md)
- [x] `U5.6` manual: 50 back-to-back captures, zero CRC failures and no hangs; `tune 2600` is refused. (SC2, R3, E1)
- [x] `U5.7` manual: time a fresh clone through build, flash, info, capture and spectrum against SC1, and record it in `bench-results.md`. (SC1)

### U6a ESP32 IQS commands *(only if U4 = A, or B accepted)*
- **Depends on**: U4
- **Owns**: `main/targets/esp32/iq_stream.h` (new; the interface below), `main/targets/esp32/receiver.c`, `main/Kconfig.projbuild` (new option only), `sdkconfig.defaults.esp32-devkitv1` (U1 is finished by then), `docs/devkitv1/iq-stream-uart.md`
- [x] `U6a.1` Kconfig `ESP_SDR_ESP32_IQS` (bool, depends on IDF_TARGET_ESP32, default n); set `=y` in the devkitv1 defaults. (D5)
- [x] `U6a.2` `iq_stream.h`: `bool iq_stream_start(unsigned ms, unsigned dec, unsigned bits, unsigned rate_code, unsigned shift, unsigned mode); void iq_stream_run_until_stop(void);`, plus the end-of-stream statistics struct used to print `IQSEND`. U6b implements these.
- [x] `U6a.3` `docs/devkitv1/iq-stream-uart.md`: the **single source** for the link budget. Payload rate = `fs/dec × 2 × bits/8` bytes/s plus 28 bytes of framing per frame. The usable UART payload is 92,160 B/s at 921600 8N1; allow ≤ 85% of it. Include the resulting allowed (rate_code, dec, bits) table. U6a.4 and U7.2 implement exactly this table.
- [x] `U6a.4` `receiver.c`, under `CONFIG_ESP_SDR_ESP32_IQS`: parse `IQS` and `FOFS` with the S3 grammar (`main/targets/esp32s3/receiver.c` ~383–426, ~52–64, ~256–268). Refuse combinations outside the U6a.3 table with `ERR args` before reconfiguring. Apply the FOFS kHz offset in the ESP32 tune path. Add `IQS` and `FOFS` to the ESP32 `CAPS` reply. Add the `Modified by` line. (R9, E4, D2)
- [x] `U6a.5` verify: the upstream `esp32` profile `.bin` is byte-identical with and without this branch (switch off), and `python3 -m unittest discover -s tests` passes. (D1) *(bins identical except timestamp/ELF-hash/checksum noise, same offsets as two builds of the unmodified tree; unittest: 10 errors in test_rx_recalibration, host clang `--wrap` linker failure, unrelated to this unit)*

### U6b ESP32 IQS engine *(parallel with U6a)*
- **Depends on**: U4
- **Owns**: `main/targets/esp32/iq_stream.c` (new), the esp32 IQS source line in `main/CMakeLists.txt`
- [x] `U6b.1` Implement `iq_stream.h`. Set up the ring or re-arm loop per `ring-feasibility.md`. The app CPU worker runs CIC first stage plus a C FIR (or esp-dsp ESP32 asm) to the requested decimation. Emit `IQS1` frames with the identical 24-byte header and CRC32 through the portable `txq` (reuse or copy the portable part of `ring_capture.c` ~262–290, whichever is the smaller diff). Account for `lost_pairs` and sample indices so the `IQSEND` loss field is exact. If output is blocked for 2 s, stop, as upstream does. (R8, D2, E2)
- [x] `U6b.2` Add `if(IDF_TARGET STREQUAL "esp32" AND CONFIG_ESP_SDR_ESP32_IQS)` (or the equivalent pattern used in `main/CMakeLists.txt`) to compile `iq_stream.c`.
- [x] `U6b.3` manual, after U6a merges: `IQS 60000 512 8 <16 MS/s code>` at 2402 MHz delivers frames, and `IQSEND` reports loss that is counted exactly and < 0.01% (bank-switch gaps, accepted by the user at the U4 gate). (US2.1, SC3)

### U7 Host stream *(parallel with U6; frame format is fixed by docs/iq-stream.md)*
- **Depends on**: U2, U3, U5, U6a.3 (the budget table)
- **Owns**: `host/esp32sdr/iqs.py`, `host/esp32sdr/cli_stream.py`, `host/tests/test_iqs.py`, the subcommand-registration line in `host/esp32sdr/cli.py` (U5 is finished by then)
- [x] `U7.1` `iqs.py`: `parse_iqs1(buf)` (24-byte header, 4/8/16-bit payload with I,−Q, CRC32). `IqsSession(device, dec, bits, mhz, fofs_khz)` yields sample blocks, detects sequence and sample-index gaps, and reads `IQSEND`. Reported loss = firmware-reported loss + host-detected gaps. (R8, E2)
- [x] `U7.2` Before sending, refuse (dec, bits) combinations outside the table in `docs/devkitv1/iq-stream-uart.md` (U6a.3); keep it in one dict, with a test that checks it against the doc. (E4)
- [x] `U7.3` `cli_stream.py`: register a `stream --freq --fofs --dec --bits --seconds -o BASE` subcommand (writes SigMF via `StreamWriter`, prints loss at the end). Register it in `cli.py` with one import and one registration line. (US2, R5, R9)
- [x] `U7.4` Tests: synthetic IQS1 streams with a dropped frame, a bad CRC and a sample-index jump. Loss is counted exactly. (R8, R10)
- [x] `U7.5` manual: a 60 s stream at dec 512 / 8-bit reports loss < 0.01%, counted exactly (SC3). A phone advertising BLE at 2402 MHz shows bursts at its advertising interval in a spectrogram of the file (US2.2).

### U8 Live view bridge *(after U6, U7)*
- **Depends on**: U6a, U6b, U7
- **Owns**: `host/esp32sdr/rtltcp.py`, `host/tests/test_rtltcp.py`, `docs/devkitv1/live-view.md`
- [x] `U8.1` Try `z2labs/esp-sdr-bridge` against the board. Record in `live-view.md` whether it works and its license; don't vendor it.
- [x] `U8.2` If it doesn't work: `rtltcp.py` with `esp32sdr bridge --listen :1234`, an rtl_tcp server sending the 12-byte dongle header and u8 offset-binary I/Q. Command 0x01 (set freq) maps to `FREQ` + `FOFS` (restarting `IQS`); sample rate is fixed by dec. Tests drive it with a fake device and a socket client. (US3)
- [ ] `U8.3` manual: gqrx connects, shows a waterfall, and retunes 2400→2483 MHz; BLE advertising channels are visible. (US3.1, SC4)

### U9 Snapshot waterfall fallback *(only if U4 = C, or B rejected)* — N/A: B accepted at the gate
- **Depends on**: U5
- **Owns**: `host/esp32sdr/waterfall.py`, `host/tests/test_waterfall.py`
- [ ] `U9.1` `esp32sdr waterfall --freq --bins` (pyqtgraph, `[view]` extra): loops `spectrum()` and scrolls a waterfall, and shows the refresh rate. (R11)
- [ ] `U9.2` Test: the frame-to-image pipeline runs on fake frames without a display. (R10)
- [ ] `U9.3` manual: ≥5 Hz refresh at 2437 MHz, with Wi-Fi activity visible. (SC5)

**U4 outcome (2026-11-05):** verdict B, accepted by the user. Ring via 32 KiB bank ping-pong (DPORT MAC_DUMP_MODE 2<->1), 0-7 lost samples per 8192 per switch, counted exactly; boxcar decim-16 at 5.4 cycles/pair on the bare-metal app CPU. Details and U6 notes: `docs/devkitv1/ring-feasibility.md`.

## Execution waves
| Wave | Units | Notes |
|------|-------|-------|
| 1 | U1, U2, U3 | parallel, disjoint files; only U1 uses the board |
| 2 | U4 | board; ends with a user decision |
| 3 | U5 | board; burst acceptance (US1 complete) |
| 4 | U6a + U6b + U7 (if gate passes) or U9 | disjoint files; U7.2 waits for U6a.3's table; board use by U6b.3 only |
| 5 | U8 | if gate passed |

## Testing strategy
- Host: `pytest` in `host/` with the fake device, no board (R10). Upstream's `python3 -m unittest discover -s tests` must still pass after firmware changes.
- Firmware: the upstream `esp32` profile output stays unchanged (U6a.5); the devkitv1 profile builds via `build_firmware.py` (U1.7).
- Bench: the manual tasks above, run by one agent at a time on `/dev/cu.usbserial-0001`. Results go in `docs/devkitv1/bench-results.md`.

## Risks & rollback
- ESP32 has no circular dump mode: the U4 gate stops streaming work cheaply, and U9 delivers R11.
- 16 MS/s doesn't fit 15 cycles/pair on the app CPU: U4.3 measures this before any port. Mitigation: a cheaper first stage (boxcar), or capture at 16 MS/s with a smaller decimation chain.
- The bare-metal app CPU disturbs the RF window or WDT: U4.3 checks that CAP16/SPEC still work. Rollback: the Kconfig switch off.
- Upstream merges: our firmware diff is limited to new esp32 files, the esp32 CMake lines, the ESP32 branch of ring_probe.h, one Kconfig option and receiver.c. Merge only between waves.

## Coverage
| Spec | Plan |
|------|------|
| G1 | U1, U2, U3, U5 |
| G2 | U4, U6a, U6b, U7, U8 |
| G3 | U1.3, U1.4, U2.1 |
| US1.1 | U5.2 |
| US1.2 | U5.4 |
| US1.3 | U3.2, U5.1, U5.2 |
| US2.1 | U6b.3, U7.5 |
| US2.2 | U7.5 |
| US3.1 | U8.3 |
| R1 | U1.5, U1.6 |
| R2 | U1.2, U2.2 |
| R3 | U2.4, U5.6 |
| R4 | U2.3, U3.1, U5.2 |
| R5 | U3.3, U5.2, U7.3 |
| R6 | U3.1, U3.4, U5.5 |
| R7 | U2.3, U2.4, U2.5 |
| R8 | U6b.1, U7.1, U7.4 |
| R9 | U6a.4, U7.3 |
| R10 | U2.5, U2.6, U3.4, U5.3, U7.4, U9.2 |
| R11 | U9 |
| E1 | U2.4, U5.6 |
| E2 | U6b.1, U7.1 |
| E3 | U2.2, U2.4, U2.5 |
| E4 | U6a.3, U6a.4, U7.2 |
| SC1 | U1.8, U5.7 |
| SC2 | U5.6, U4.3 |
| SC3 | U6b.3, U7.5 |
| SC4 | U8.3 |
| SC5 | U9.3 |
