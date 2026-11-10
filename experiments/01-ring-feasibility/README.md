# 01 · Can the classic ESP32 capture without gaps?

esp-sdr captures on the classic ESP32 in bursts. The radio writes 16380 I/Q samples into a 64 KiB window of SRAM and stops, which is about 1 ms of signal at 16 MS/s. Everything I wanted to do later, streaming and decoding, needs the radio to keep writing while the CPU reads what it already wrote. esp-sdr does that on the ESP32-S3, but I hadn't seen it done on the classic ESP32. So before building a stream I wanted to know whether the hardware allows it at all, and if not, how close it gets.

I built a probe firmware (the DevKit V1 profile with `-DRING_PROBE=ON`) with commands that set the capture registers one at a time and report what happens, tuned to 2412 MHz with the hardware AGC, receive only.

The first question was whether the writer can loop. With control bit 17 set it wraps around its count instead of stopping. In 1 ms the write pointer wrapped 4, 2 and 0 times at 80, 40 and 16 MS/s, which is what the rates predict for a window of about 16k samples (80,000, 40,000 and 16,000 samples in that millisecond).

The second question was whether the CPU can read the window while the radio writes it. At first the probe reported all 8 words it sampled as live (`live=8`), but that was an artefact: while the radio owns a part of the window, CPU reads of it return the constant `0xa55aa55a`, not memory. Looking closer, the 64 KiB window is two 32 KiB banks, and the `MAC_DUMP_MODE` field in DPORT decides who owns which: mode 2 gives the radio the lower bank, mode 1 the upper, 3 both and 0 neither. In modes 1 and 2 the writer's offsets 0 to 8191 land in that mode's bank, and offsets from 8192 up are dropped, not wrapped into the other bank. I found that with a 16384-word ring in mode 3, where the upper bank only received the writes from the margin around each switch.

That allows ping-pong buffering. The writer runs an 8192-word ring in one bank while the CPU reads the other, and when the pointer wraps, the CPU flips the mode so the two swap. The cost is at the seam. The flip lands *k* samples after the wrap, so the first *k* samples of the new lap go to the start of the old bank and overwrite the first *k* samples of the lap before. Each bank is filled with a sentinel pattern beforehand, so the firmware can count *k* exactly: it is the number of sentinel words left at the start of the new bank. Over 400, 1000 and 2000 swaps at 16, 40 and 80 MS/s, every bank came back fully written (`sparse_missing=0`), the CPU never fell a lap behind (`overrun=0`), and *k* averaged 0.21, 1.64 and 3.50 samples per 8192, with a maximum of 1, 5 and 7 (3 at 16 MS/s in a repeat run). That is 0.0026%, 0.020% and 0.043% of the samples. The poll that flipped the mode ran on the main CPU with interrupts masked, so *k* depends on how quickly that loop notices the wrap.

For comparison I also timed the simpler fallback, re-arming a one-shot capture each time it finishes. The gap between captures was 147, 156 and 159 CPU cycles at 240 MHz, which is 9.8, 26 and 53 samples at 16, 40 and 80 MS/s, so 99.94%, 99.84% and 99.68% of the time covered with 16380-sample captures. Ping-pong loses 10 to 50 times less.

The third question was whether there is enough CPU to filter the samples as they arrive. The firmware runs FreeRTOS on one core, so I started the second core bare-metal, outside the RTOS. A decimate-by-16 boxcar over 16368 captured pairs cost 5.37 cycles per pair on that core (6.12 with sign extension), against the 12 cycles per pair I had set as the limit for keeping up. At 16 MS/s one 8192-pair bank lasts 512 µs and takes about 184 µs to filter, 36% of the core. Its output matched the same filter on the main core bit for bit. Afterwards, with the second core still running its loop, normal burst captures at all three rates (CRC good) and the on-chip spectrum still worked. Two probe runs that kept interrupts masked for about 400 to 500 ms tripped the interrupt watchdog and reset the board, so any such loop has to stay well under that; I noted 250 ms as the limit.

So the answer was "almost": a ring with small gaps at each swap, each one counted. Counted gaps were acceptable for what I needed, because anything downstream can see exactly where samples are missing, so I built the stream on it in [03](../03-iq-stream/README.md). Two things I only learned while building that stream: reading one bank while the radio writes the other is about twice as slow, around 10 cycles a pair instead of 6, so the real filter moved to the main core; and the final firmware swaps banks on a fixed schedule a little before each lap ends instead of polling for the wrap, so in normal running the seam loses nothing (03 counted 0 lost pairs in 60 s).

This experiment doesn't show that the samples are continuous across a seam. The sentinel only shows which words were written. A tone recorded through a seam and checked for phase continuity would show it, and I didn't do that here.

A side finding on the host: the Python transport set pyserial's timeout before every read, and on this board's CP2102 link that dropped bytes, so replies like `VERSION?` timed out and capture headers came back corrupted. The probe script reads replies itself to get around it; the transport was fixed in [02](../02-burst-capture/README.md).

---

## Numbers

| What | Value | Where |
|---|---|---|
| Ring wraps in 1 ms at 80 / 40 / 16 MS/s | 4 / 2 / 0 | `RINGPROBE` in notes.md |
| Seam loss *k* per 8192-sample swap, mean / max, at 16 / 40 / 80 MS/s | 0.21 / 1, 1.64 / 5, 3.50 / 7 | `RINGPP8` in notes.md |
| Re-armed one-shot, gap per capture | 147 / 156 / 159 cycles (9.8 / 26 / 53 samples) | notes.md |
| Boxcar /16 on the bare-metal core | 5.37 cycles per pair (`raw_sum`), pass mark 12 | `DSPPROBE` in notes.md |

## Files

- [notes.md](notes.md): the full original notes, including the raw probe output of the final run (13:01) and how to read it.
- `scripts/probe_ring.py`: sends the probe commands and logs the replies. Needs the board and a `-DRING_PROBE=ON` build (probe code: `main/diagnostics/ring_probe.h`).
- No `data/`: the raw replies are printed in notes.md ("Raw output"); the log file itself (`/tmp/ring.log`) was not kept.

## How to re-run

With the board connected, build and flash the probe profile as in notes.md ("Reproduce"), then:

```sh
host/.venv/bin/python experiments/01-ring-feasibility/scripts/probe_ring.py --help
```

Reflash the plain profile afterwards (see [docs/devkitv1/README.md](../../docs/devkitv1/README.md)).

## Ran

2026-11-05 12:49–13:04 EST (build dir 12:49, raw-output header 13:01:51, notes 13:04). Commit 85f0fd5 (15:17).

[← 00: prior work](../README.md#00-prior-work) · [index](../README.md) · [02-burst-capture →](../02-burst-capture/README.md)
