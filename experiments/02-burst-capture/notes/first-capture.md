# Burst capture at 2437 MHz: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## Link fix (prerequisite)

`SerialTransport` used to set `serial.Serial.timeout` before every read (one byte at a time for
`readline`). On this CP2102 link that dropped bytes: `VERSION?` timed out, CAP16 headers and
SPC1 frames came back corrupted. It now opens the port once with a fixed 50 ms timeout, reads
whatever is waiting in bulk into a buffer, and enforces deadlines itself. After the change:
`VERSION?`, `INFO`, `LIMITS?`, `TRANSPORT?` (`TRANSPORT UART 921600`) and `CAPS` answer through
`Device`, and every capture below passed its CRC.

## U5.4 capture at 2437 MHz (US1.2)

```
$ esp32sdr info
chip      ESP32SDR protocol 6 mode burst
max_n     16380
firmware  1d27d848…-dirty (esp32-devkitv1, 2026-11-05)
transport UART 921600
rates     80 40 16 MS/s
bits      8 10
gain      index 0..81 (step 1), hardware AGC default
tuning    1850-2500 MHz (host limit)
caps      VERSION GPIO SPEC SPECN SPECCAPS SPECSTAT DCT UARTBAUD RXLIMITS SERIALLEASE TUNEEXT RX40 RX16 LPFANA GAIN HWAGC IQ8

$ esp32sdr capture --freq 2437 --rate 40 --n 16380 -o ch6 --psd figures/ch6-capture-psd.png
captured 16380 samples at 40 MS/s, 2437 MHz, 8-bit, CRC ok, 0.36 s -> ch6.sigmf-data
peak 2446.297 MHz 17.4 dB, median -5.1 dB (DC ±0.2 MHz excluded)

$ esp32sdr spectrum --freq 2437 --bins 1024 --rate 40 --ms 1000 -o figures/ch6-spec.png
spectrum 1024 bins at 40 MS/s, 2437 MHz: 86 frames, 256 FFTs, mean detector
peak 2437.781 MHz 48.1 dB, median 38.3 dB
$ esp32sdr spectrum ... --detector max -o figures/ch6-spec-max.png
spectrum 1024 bins at 40 MS/s, 2437 MHz: 87 frames, 257 FFTs, max detector
peak 2426.180 MHz 74.0 dB, median 57.0 dB
```

The single 410 µs burst ([ch6-capture-psd.png](../figures/ch6-capture-psd.png)) caught a packet
occupying ≈2440–2449 MHz, about 20 dB above the rest of the span, inside 2427–2447 MHz and its
edge. The 1 s on-chip spectra ([mean](../figures/ch6-spec.png), [max-hold](../figures/ch6-spec-max.png))
show energy across 2427–2447 MHz, with the flat-topped shape of the analog/decimation filter
(≈±8–10 MHz) that stage 0 already described, so the shape alone is not proof of Wi-Fi. The
packet in the burst capture is. US1.2: **pass**.
