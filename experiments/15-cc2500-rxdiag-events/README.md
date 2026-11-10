# 15 · Counting short bursts with the drone's own CC2500

Everything so far was measured with the ESP32 alone. I wanted a second receiver to check things against, and the obvious one was the CC2500 radio chip on the drone's own flight controller, a Tinyhawk. I had written a receive-only diagnostic firmware for it earlier (`rxdiag`, now in [esp32-d8tx tools/cc2500-rxdiag](https://github.com/mb6611/esp32-d8tx/tree/main/tools/cc2500-rxdiag)), but its stream mode couldn't see short bursts, and short bursts are what I would be looking for.

The old mode read the chip's signal strength (RSSI) in a loop. One attempt, two reads over SPI, took 19.3 µs, but 68% of attempts were thrown away because the two reads differed, which the CC2500's read rule requires. So in practice it got a point sample about every 60 µs, with random holes, and each 10 ms window kept only its minimum and maximum. A burst shorter than about 60–100 µs was caught only by chance, and the count, start time and length of bursts were lost. The chip itself updates RSSI about every 10 µs at this bandwidth, so the limit was my loop, not the chip.

So I added an "events" mode. The CC2500 can drive one of its pins (GDO0) as carrier sense: high while the RSSI is above a threshold. On this board that pin is wired to an interrupt-capable input, so an interrupt on both edges timestamps every rise and fall into a 256-entry ring, with no polling gap. The main loop still samples RSSI for each burst's peak level, and writes one record per burst and a cumulative counter record per 100 ms window, so a lost record never loses counts. Before receiving it checks the pin wiring and reads back every configuration byte. The motors stay disabled, and the firmware never transmits.

First I had to pick a threshold, using 3 s of ambient traffic at 2402 MHz per setting. At the highest gain the carrier-sense line was stuck high and gave no edges at all. With the gain settings 203 and 251 and the absolute threshold at 0 or +7 I got between 94 and 4 bursts a second, and at 255 with +7, none. I used 251 with threshold 0.

For the test, the flight controller was on USB only, and the ESP32 ran ESP-IDF's Bluetooth example `bleprph`, which advertises until something connects. I switched it off and on by holding the chip in reset through the USB adapter's RTS line, which is wired to its EN pin. I counted bursts at 2402 MHz, Bluetooth advertising channel 37, for 10 s at a time, alternating off and on three times. With the advertiser off there were 23.7–27.5 bursts a second, with a median length of 266–286 µs; I think these are other Bluetooth devices nearby, but I didn't show that. With it on there were 38.0–41.2 a second. After seeing the data I picked a peak level of 50 to separate strong bursts: there were none in any off run and 163–168 in each on run, a median 64–65 ms apart, which fits a regular advertising interval. At 2414 MHz, where no advertising channel is, there were no bursts at all, on or off. At 2426 MHz (advertising channel 38) the strong-burst count went from 0 to 19.

The accounting held: across the six A/B runs the ring never overflowed, 3 burst records failed to send and 2 window records were lost, and the cumulative counters kept the totals intact. Each 10 s run caught 10–63 bursts shorter than 100 µs, many of them with no RSSI sample at all, which the old polling loop would have missed.

This counts energy only. Nothing was demodulated, so the link between those bursts and the ESP32 rests on the on/off alternation, the channel selectivity and the 64 ms spacing, not on packet content. The threshold isn't calibrated in dBm, and I didn't compare the two antennas. Afterwards I put the stock Betaflight back by DFU and restored the settings; the final `diff all` is identical to the one from before the flash (287 lines), although the replay log itself ends in a USB error when the board re-enumerated. The drone's radio did become my test receiver: it is how I checked the ESP32's transmissions in [esp32-d8tx](https://github.com/mb6611/esp32-d8tx).

There are no figures for this experiment.

---

## Numbers

| 2402 MHz, 10 s runs | Bursts/s | Bursts with peak ≥ 50 (threshold chosen after seeing the data) | Median spacing of those | Median length |
|---|---|---|---|---|
| Advertiser off, 3 runs | 27.5 / 23.7 / 26.0 | 0 / 0 / 0 | – | 266–286 µs |
| Advertiser on, 3 runs | 38.0 / 40.1 / 41.2 | 164 / 168 / 163 | 64–65 ms | 305 µs |

Control at 2414 MHz: 0 bursts, off and on. 2426 MHz: 22.5 vs 39.5 bursts/s, peak ≥ 50: 0 vs 19. Source: `data/summaries.jsonl`, [data/README.md](data/README.md).

## Files

- Notes: [data/README.md](data/README.md), the original results notes.
- `data/`: A/B logs (`ab/`), threshold sweep (`sweep/`), `summaries.jsonl`, flash and restore records, `restore_replay.log`. See [data/DATA.md](data/DATA.md).
- `scripts/rxdiag_events.py`: logger and `summary` (offline on the logs). `scripts/run_ab.sh`: the A/B driver, with `scripts/esp_ctl.py`, which switches the ESP32 through its reset line (port in `ESP_PORT`). I ran them from another folder; the copies here use the repo's paths and write to `data/ab/`.
- The diagnostic firmware is in esp32-d8tx: [tools/cc2500-rxdiag](https://github.com/mb6611/esp32-d8tx/tree/main/tools/cc2500-rxdiag).

## How to re-run

```sh
cd experiments/15-cc2500-rxdiag-events/data
../../../host/.venv/bin/python ../scripts/rxdiag_events.py summary ab/*[0-9].txt sweep/*.txt smoke-2402.txt   # = summaries.jsonl
```

## Ran

2026-11-05 19:04–19:29 EST (source copy 19:04, log stamps 2026-11-06T00:15–00:19Z = 19:15–19:19 EST, notes 19:29). Committed 2026-11-06 13:39 as ad4fa09 (in esp32-d8tx: 40d27e4, 3be556c).

[← 14-hop-follow](../14-hop-follow/README.md) · [index](../README.md) · [esp32-d8tx →](https://github.com/mb6611/esp32-d8tx)
