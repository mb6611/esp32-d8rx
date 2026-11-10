# D8 single-channel stream: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## FrSky D8 (EMAX E8 transmitter) detection, 2026-11-05
Receive only. E8 ~0.5-1 m from the board, drone off. Hop table, TX id and offset taken from the drone's Betaflight backup (`diff all`: `frsky_spi_bind_hop_data`, 47 entries). Channel n = 2404.000 MHz + n x 299.93 kHz (CC2500 FREQ 0x5C7627, CHANSPC 0x23/0x7A per Betaflight `cc2500_frsky_shared.c`); D8 modem: GFSK ~31 kbaud, +-31.7 kHz deviation.
3. Single-channel stream (2421 MHz + FOFS 31, dec 256, 4-bit, shift 10): bursts recur with the hop cycle; the 4-bit default shift (11) rounded almost everything to 0, so `IqsSession` gained an optional `shift`. The single-channel fold picked 425.95 ms, which is wrong (the wideband fit above is authoritative); two strong slots per cycle there are not yet explained.
