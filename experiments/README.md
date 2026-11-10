# Experiments

These are the bench measurements behind this repo, in the order I started them. The firmware is [ESPARGOS esp-sdr](https://github.com/ESPARGOS/esp-sdr) with my DevKit V1 extensions, and everything ran on one ESP32 DevKit V1, receive only, on 2026-11-05 (times are EST). The question running through them is whether a $5 ESP32 can receive, and then decode, the FrSky D8 link from my EMAX E8 radio controller. Experiment 15 used the drone's own CC2500 radio as a second receiver. The transmit follow-up, where the ESP32 takes the E8's place, is [esp32-d8tx](https://github.com/mb6611/esp32-d8tx).

Several experiments ran in parallel, sharing the board one at a time; each README says where they overlap. The order comes from file times and logs, not from the commits, which came in batches later that day. The write-up of the same day, as I wrote it then, is in [docs/lab-log](../docs/lab-log/README.md).

Each folder has a `README.md` (the story first, then numbers, files, how to re-run and when it ran), the original detailed notes where there were some (`notes.md`, or `notes/` where one experiment groups several checks), `data/` with the raw data as recorded, `scripts/`, and `figures/`. Files over 1 MB (the recordings and raw captures, about 790 MB) are not in git but in the release asset `experiments-data-v1`; each `data/DATA.md` lists them with size and sha256, and `experiments/fetch-data.sh` downloads and unpacks them into place.

## 01: [Can the classic ESP32 capture without gaps?](01-ring-feasibility/README.md)

2026-11-05 12:49–13:04. esp-sdr only captured continuously on newer chips, so I probed whether the classic ESP32's sample writer can loop round a buffer like a ring. It can, but the CPU can't read the half the radio is writing into, so I swapped two 32 KiB halves each time the writer wrapped. That loses 0–7 samples per 8192-sample swap, and the firmware counts them exactly; the filter on the second core costs 5.37 cycles per sample pair of a 12-cycle budget.

## 02: [Burst capture: first look, I/Q orientation, robustness](02-burst-capture/README.md)

2026-11-05 13:09–13:28. Three quick checks of esp-sdr's burst capture on this board. A capture at 2437 MHz caught a Wi-Fi packet about 20 dB above everything else; an LO sweep from 2407 to 2477 MHz showed the host's I/Q is the right way round (correlation +0.218 against −0.007, 32 of 39 LO pairs) while the on-chip spectrum was mirrored. 350 captures in a row had no failures, each taking about 0.36 s, the time to move the bytes at 921600 baud.

## 03: [A continuous I/Q stream over the UART](03-iq-stream/README.md)

2026-11-05 13:33–13:37. Using the ring from 01, the firmware filters and decimates on the second core and sends numbered, checksummed frames over the 921600-baud link. A 60 s stream at 2402 MHz, 31.25 kS/s and 8 bits delivered 1,874,990 samples with none lost, corrupted or out of order.

## 04: [rtl_tcp bridge, scripted check](04-rtltcp-bridge/README.md)

2026-11-05 13:37. The host pretends to be an rtl_tcp server so ordinary SDR programs can show the stream. A scripted client read the header, measured about 31.1 kS/s against 31.25 expected, retuned three times and reconnected, with 0 lost samples on every stop.

## 05: [Live view in gqrx and SDR++, and a bridge fix](05-live-view-sdrpp/README.md)

2026-11-05 13:47–14:05. When gqrx paused its display it stopped reading the socket, the bridge blocked, the USB serial buffer overflowed and the stream died (`short read 1006/1028`). With a separate sender thread and a bounded queue, a client that stopped reading for 40 s cost 1735 dropped display blocks and 0 stream samples, and SDR++ ran over 2.5 minutes with retunes and 0 lost samples.

## 06: [Seeing a phone's Bluetooth advertising](06-ble-advertising/README.md)

2026-11-05 14:10–14:24. I recorded 20 s at 2480 MHz (Bluetooth advertising channel 39) with my iPhone's advertiser off and then on: 27 bursts against 259. Half of the strong pulses in the on recording chain into one train with a 29.5 ms period plus Bluetooth's random 0–10 ms delay, against 4% chaining by chance with it off.

## 07: [Seeing the D8 hop channels in max-hold](07-d8-maxhold/README.md)

2026-11-05 14:44–14:45. A 5 s max-hold over 80 MHz, with the E8 on and off, checked whether its energy sits on the 47 hop channels from the drone's Betaflight backup. With it on, those channels stood out by +10.8 dB on average while the 199 unused positions on the same grid stayed at −0.1 dB; with it off, both were at +0.1 dB.

## 08: [Listening to one D8 channel](08-d8-one-channel/README.md)

2026-11-05 14:55–14:58. I streamed 20 s at 2421.031 MHz, 62.5 kS/s, 4-bit, to see single packets and time the E8's returns. The channel had 377 bursts in 20 s, far more than the E8's one visit per cycle, and the best fold, 425.95 ms, turned out to lock onto something else; the E8's real cycle is 422.50 ms. The recording later gave the first decoded packets in 11.

## 09: [Seeing the D8 hop cycle](09-d8-hop-cycle/README.md)

2026-11-05 15:00–15:05. 835 timestamped wideband snapshots in 20 s, folded like a strobe, rebuilt one hop cycle: 422.70 ms, 47 slots of 8.994 ms, with slot *k* on table entry *k*. The matched score was 23.4 dB against 4.5 ± 0.2 dB for random slot orders (3.1 against 2.9 with the E8 off). The follower in 14 later measured the cycle more precisely, 422.50 ms.

## 10: [Can the board retune fast enough to hop?](10-fast-retune/README.md)

2026-11-05 15:40–16:11. A normal tune takes 25–60 ms because it recalibrates; the synthesizer move alone (capacitor-code release plus a ROM call) took 257 µs median, 265 µs at most, and the oscillator is on the new frequency when the call returns. One calibration at 2404 MHz served all 47 D8 channels: the crystal-harmonic ruler line landed within 4.1 kHz, inside one FFT bin, with the same DC and SNR as a recalibrated tune.

## 11: [D8 decoding: from a first packet to 47 of 47](11-d8-decoding/README.md)

2026-11-05 15:40–17:07. A decoder that models the whole receiver chain found 31 CRC-valid packets (transmitter ID 66,178) in the 4-bit recording from 08. With 1-bit samples, tuning 4 MHz off to the side (mode 2) gave 22 packets against 3. The E8 turned out to transmit at 2421.032 MHz, 64 kHz below nominal; with that centre, a fixed gain of 40 and a narrower decoder filter, a 20 s recording decoded 47 of 47 visits.

## 12: [1-bit samples to fit FSK through the UART](12-one-bit-stream/README.md)

2026-11-05 15:57–16:22. FSK carries its information in the phase, so the firmware now keeps only the signs of I and Q, four samples per byte: 250 kS/s, 8 samples per D8 bit, in 69.7% of the link (at 125 kS/s a harmonic of one tone lands on the other). After rewriting the filter's inner loop in Xtensa assembly, core 0 was about 50% busy, and three 20 s runs lost no samples.

## 13: [Live decoding on one channel](13-d8live-one-channel/README.md)

2026-11-05 17:09–17:29. `esp32sdr d8live` decodes the 1-bit stream as it arrives and, once locked, only looks at a 20 ms window around each expected visit. A 274 s run decoded 649 CRC-valid packets, 2.37 a second, practically every visit, using 4.9% of the samples and about 5% of a core, and drove the sticks on a web page.

## 14: [Following all 47 hops live](14-hop-follow/README.md)

2026-11-05 17:39–18:52. The board retunes on a schedule counted in stream samples, which the Mac corrects from a line fit through the decoded packet times. A 630 s run decoded 108.5 packets a second, 97.8% of 69,926 slots, with no lock losses, and measured the hop slot at 8.989 ms. One channel, 2411.435 MHz, almost never decodes because its carrier jumps by 8–20 kHz inside the packet, which looks like the transmitter, though I haven't proved it.

## 15: [Counting short bursts with the drone's own CC2500](15-cc2500-rxdiag-events/README.md)

2026-11-05 19:04–19:29. A new "events" mode in the drone's receive-only diagnostic firmware timestamps the CC2500's carrier-sense pin by interrupt, instead of polling RSSI every 60 µs or so. With an ESP32 Bluetooth advertiser switched on and off at 2402 MHz, it counted 163–168 strong bursts per 10 s, 64–65 ms apart, against none with it off and none at 2414 MHz. It counts energy only; nothing was demodulated.

## Re-running

Run `experiments/fetch-data.sh` once to put the large files in place (needs curl and zstd, about 200 MB download). Scripts run from the repo root with the host venv (`host/.venv/bin/python`, see [docs/devkitv1/README.md](../docs/devkitv1/README.md)). Offline scripts read `../data/` next to them (`../data/<check>/` in the grouped experiments 02 and 11) and write figures to `../figures/`; set `OUT=/some/dir` to write elsewhere. Scripts that talk to the board use `/dev/cu.usbserial-0001` and write new recordings to `/tmp`, never into `data/`.
