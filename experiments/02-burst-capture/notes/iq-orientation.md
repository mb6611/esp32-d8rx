# I/Q orientation: original notes


*Section of the original `docs/devkitv1/bench-results.md` (2026-11-05), split out per experiment; only paths were changed.*

Board: ESP32 DevKit V1 (ESP32-D0WD-V3, CP2102) on `/dev/cu.usbserial-0001`, firmware profile
`esp32-devkitv1` (921600 baud), stock PCB antenna, indoors with ambient 2.4 GHz traffic
(Wi-Fi networks seen by the Mac on channels 1, 3 (40 MHz), 6 and 11). Hardware AGC (default),
so levels are uncalibrated dB. Measured 2026-11-05. Host: `host/` package, `esp32sdr` CLI.

All commands below run from `host/` with the venv active (`. .venv/bin/activate`).

## U5.5 orientation (R6)

What the plan asked: tune 10 MHz below an active channel and see energy at +10 MHz. With
ambient traffic only, that single-LO test was **ambiguous**: the band is busy on both sides of
every LO (Wi-Fi on 1/3/6/11, plus Bluetooth hopping), and bursts are sparse (≈6–20 % of 410 µs
captures). Burst-gated counts, 120 captures per LO, bursts > 6 dB over the typical level and
dominant on one side (±5..15 MHz):

| LO (MHz) | expected side | upper | lower |
| --- | --- | --- | --- |
| 2452 (ch11 − 10) | upper | 17 | 21 |
| 2472 (ch11 + 10) | lower | 14 | 18 |
| 2427 (ch6 − 10) | upper | 7 | 5 |

So a controlled test was used instead: an LO sweep. The LO steps 2407→2477 MHz in 5 MHz
steps; at each LO the host takes a max-hold PSD of 25 CAP16 bursts (decoded with
`decode_cap`, Q negated) and a 2 s on-chip `SPEC` max-hold. Real RF features stay at the same
RF frequency as the LO moves only under the correct mapping. Correlating spectra of LO pairs
5–15 MHz apart (after removing the fixed baseband shape):

| data | RF = LO + f | RF = LO − f | LO pairs favouring LO + f |
| --- | --- | --- | --- |
| host capture (`decode_cap`) | **+0.218** | −0.007 | 32 / 39 |
| on-chip SPEC bins (raw order) | −0.060 | **+0.276** | 4 / 39 |

[orientation-sweep.png](../figures/orientation-sweep.png) shows the four mappings. Conclusions:
- Host I/Q (`decode_cap`, Q negated) is correctly oriented: RF above the LO is at +f. **R6 pass.**
- The firmware's SPEC bins are the FFT of the raw `(I, −Q)` words, i.e. mirrored:
  natural bin +f is RF LO − f. `spec_cmd.SpecRun.rf_axis_hz` applies this, matching
  `tools/s3_ring.py`.

Reproduce: `scripts/orientation_sweep.py` (≈4 min). A cleaner literal check
needs a controlled emitter on a known channel (e.g. a phone hotspot pinned to channel 1 with
traffic, tune 2402).
