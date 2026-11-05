# ESP32 DevKit V1 receive-only SDR

## Problem
The classic ESP32 DevKit V1 can work as a 2.4 GHz receiver with ESPARGOS esp-sdr firmware. Today that only works through one-off scripts in the auto-drone project: the firmware is built from an unmodified upstream clone with local overrides, and host access is a single capture script. There is no reusable, licensed, self-contained tool for using this board as a bench receiver. Upstream also offers no continuous I/Q on the classic ESP32. It has burst captures only, so the board can't drive a live waterfall or a long recording.

## Goals
- `G1` Use the DevKit V1 as a **2.4 GHz bench receiver from one repo**: build, flash, tune, capture and inspect spectra, with nothing borrowed from other projects.
- `G2` Get **continuous narrowband I/Q** from the board over its own USB port, for long recordings and live viewing in standard SDR software.
- `G3` **Redistributable cleanly**: anyone can clone, build and share it within the upstream license.

## Non-goals
- `NG1` **Transmit** of any kind.
- `NG2` Chips or boards **other than the classic ESP32 DevKit V1**.
- `NG3` **Extra hardware** (a faster USB-serial adapter, a signal generator, LNAs).
- `NG4` **Wideband continuous streaming** (a whole Wi-Fi channel live). The board's USB-serial link caps continuous bandwidth to tens of kHz.
- `NG5` Calibrated absolute power. Gain stays as uncalibrated as upstream.

## User stories
### US1 (P1) Bench capture
As the bench engineer I want to tune the board and grab I/Q bursts and spectra from a command line, so I can see what's on the air in 2.4 GHz. Why P1: it's the baseline the rest builds on.
- `US1.1` GIVEN a freshly flashed board WHEN I ask for device info THEN I get firmware version, chip and limits.
- `US1.2` GIVEN a board tuned to Wi-Fi channel 6 WHEN I capture a burst THEN I get a file that standard SDR tools open, and its spectrum shows activity around 2437 MHz.
- `US1.3` WHEN I ask for a spectrum THEN I get one on-board computed snapshot without transferring raw I/Q.

### US2 (P2) Continuous recording
As the bench engineer I want a gapless narrowband I/Q stream for as long as I choose, so I can watch intermittent signals over time. Why P2: it's the new capability, but it depends on hardware feasibility.
- `US2.1` GIVEN a stream at ≥31 kS/s complex, 8-bit I and Q WHEN it runs 60 s THEN zero samples are reported lost.
- `US2.2` GIVEN a phone advertising BLE WHEN I stream at 2402 MHz THEN energy bursts repeating at the advertising interval are visible in the recording's spectrogram.

### US3 (P3) Live view
As the bench engineer I want to see the stream live in gqrx or SDR++ and retune from there. Why P3: convenience on top of US2.
- `US3.1` WHEN gqrx connects and I retune across 2400–2483 MHz THEN the waterfall follows.

## Requirements
- `R1` THE SYSTEM SHALL build and flash **from a fresh clone with one documented command each**, using a pinned toolchain.
- `R2` THE SYSTEM SHALL talk to the board **only through the DevKit's built-in USB-serial port**, at up to 921600 baud.
- `R3` THE SYSTEM SHALL accept tuning requests **from 1850 to 2500 MHz inclusive** and refuse any outside that range. IF the receiver fails to lock THEN it SHALL report the lock failure instead of returning data.
- `R4` THE SYSTEM SHALL capture **bursts at every rate and bit depth upstream supports on the ESP32** (80/40/16 MS/s; 8 and 10-bit), with integrity checks on every transfer. Device info SHALL list them.
- `R5` THE SYSTEM SHALL save captures and streams as **SigMF**, recording centre frequency, sample rate, bit depth, gain/AGC state and the firmware version.
- `R6` THE SYSTEM SHALL return samples with **correct I/Q orientation**: RF above the LO shows at positive frequency. Any upstream sign quirks are handled internally. Checked by tuning 10 MHz below an active Wi-Fi channel: its energy appears at +10 MHz.
- `R7` IF a transfer fails its integrity check or times out THEN THE SYSTEM SHALL **report it and recover the link** without a manual board reset.
- `R8` WHILE streaming, THE SYSTEM SHALL **account for every lost sample**, and SHALL report zero only when nothing was lost.
- `R9` WHILE streaming, THE SYSTEM SHALL support **fine tuning in 1 kHz steps**.
- `R10` THE SYSTEM SHALL work **with no hardware attached** for everything except talking to the board, and its tests SHALL run that way.
- `R11` IF continuous capture is infeasible (`D4`) THEN THE SYSTEM SHALL show a **live waterfall from repeated on-board spectra**, refreshing at ≥5 Hz.

## Constraints & assumptions
- `C1` Firmware is **adapted from ESPARGOS esp-sdr**, so the repo is **GPL-3.0-or-later**. Upstream attribution, the fork point, and the Apache-2.0 notices for third-party parts must ship with it.
- `C2` Hardware is the user's ESP32 DevKit V1 (classic ESP32, CP2102 bridge), plus a Mac host.
- `C3` *Assumed:* the board's capture hardware can run continuously on the classic ESP32. Upstream only proves this on newer chips. US2 and US3 depend on it.
- `C4` Test signals are only what's around: Wi-Fi access points and phone BLE.

## Key decisions
- `D1` **Fork esp-sdr with full history** and keep upstream mergeable — because licensing, attribution and upstream fixes come for free, and the ESP32 streaming work could go back upstream. Rejected: copying just the ESP32 code (loses easy merges); submodule plus overlay (the changes sit inside upstream's main component).
- `D2` **Stay wire-compatible with upstream's S3 stream format** — because existing upstream host tools, such as the third-party gqrx/SDR++ bridge, may then work as-is. Rejected: a custom UART-tuned protocol.
- `D3` **Self-contained Python host tooling** — because it reuses the auto-drone capture code and the scientific Python stack. Rejected: depending on auto-drone.
- `D4` **Continuous streaming is gated by a feasibility check** — IF the classic ESP32 can't capture continuously, THEN US2 and US3 are dropped. A live view built from repeated snapshot spectra replaces them (`R11`). The check passes when the board captures continuously into its ring for 60 s with the second core running, and burst capture and spectra still meet `SC2`.

- `D5` **The board is a new upstream-style build profile**, not a separate build system — because upstream already builds variants from its target catalog with per-profile config (the S31 streaming build is one), so the 921600 baud setting and later the ESP32 stream are config switches that upstream could take as-is. Rejected: our own build scripts and presets.

## Structure
```text
esp32-devkitv1-sdr/          fork of esp-sdr (D1); upstream tree stays in place
├── main/ components/ ...    upstream firmware; ESP32 changes kept small, marked, behind config
├── firmware-targets.json    + a DevKit V1 build profile alongside upstream's esp32 one (D5, R1)
├── host/                    Python package + CLI: device link, capture, spectrum, stream, SigMF (D3)
│   └── tests/               run with no board attached (R10)
├── docs/devkitv1/           bench notes, measured limits, streaming design
├── LICENSE  NOTICE.md       GPL-3.0 + upstream attribution and third-party notices (C1)
└── .ru/                     specs, plans, project memories
```

## Edge cases
- `E1` Tuning outside the range, or a failed lock: refused with a clear error and no data (`R3`).
- `E2` Host reads too slowly during streaming: the board drops and counts samples, and doesn't stall or crash (`R8`).
- `E3` Board left in a bad state, e.g. by a previous session or other firmware: the host resyncs or resets it over the serial control lines.
- `E4` A stream rate or bit depth that the link can't carry: refused up front, with the supported combinations listed.

## Success criteria
- `SC1` From a fresh clone: build, flash, info, capture and spectrum all work on the bench in under 10 minutes of commands.
- `SC2` 50 back-to-back bursts complete with zero integrity failures.
- `SC3` (if `C3` holds) A 60 s stream at ≥31 kS/s completes with lost samples counted exactly and under 0.01% (bank-switch gaps; accepted at the U4 gate, 2026-11-05).
- `SC4` (if `C3` holds) gqrx or SDR++ shows a live waterfall and retunes across the 2.4 GHz band.
- `SC5` (if `C3` fails) The `R11` waterfall shows Wi-Fi channel activity live at ≥5 Hz.
