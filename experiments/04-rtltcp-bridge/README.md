# 04 · rtl_tcp bridge, scripted check

With the stream working ([03](../03-iq-stream/README.md)), I wanted to watch it in ordinary SDR programs instead of my own plots. Many of them speak rtl_tcp, the network protocol of the cheap RTL-SDR dongles: the client connects over TCP, receives a 12-byte header (`RTL0`, the tuner type and the number of gain steps), then a continuous stream of 8-bit I/Q, and sends 5-byte commands back, such as 0x01 to set the frequency. So `esp32sdr bridge` pretends to be such a dongle. It runs the stream from the board, passes the samples on to the client, and when the client asks for a new frequency it stops the stream, retunes the board and starts it again.

Before trying a real viewer, I checked the bridge with a small Python client, so that I could see exactly what came over the socket. I started the bridge on 127.0.0.1:1234 at 2402 MHz, dec 512, 8-bit (31.25 kS/s). The client read the header, measured the sample rate over 10 s, sent three retunes (2400, 2483 and 2426 MHz) with a 6 s read after each, disconnected, then connected a second time and measured another 10 s.

The header was right: `RTL0`, tuner type 5, 29 gains. The measured rates were 31,086 and 31,181 S/s in the two 10 s windows, against 31,250 expected. After each retune the samples kept coming. Those reads came out at about 38.7 kS/s, but only because the client slept 1.5 s after each command and then drained the backlog that had piled up. The bridge logged each tune, and each time it stopped the stream to retune, the stream reported 0 lost samples: five stops in all, across the two connections. The second client got the header and about 31.2 kS/s at the last frequency, 2426 MHz. Afterwards `esp32sdr info` answered normally. I found no bugs and changed no code, and the host tests passed (62).

This only shows that the bridge works with a client that reads all the time. It says nothing about a client that stops reading, which is what gqrx does when its DSP is off, and that broke it in [05](../05-live-view-sdrpp/README.md).

---

## Numbers

| What | Value | Where |
|---|---|---|
| Header | `RTL0`, tuner type 5, 29 gains | notes.md, `scripts/client.py` |
| Rate over 10 s, first / second client | 31,086 / 31,181 S/s (31,250 expected) | notes.md |
| Retunes | 2400, 2483, 2426 MHz | `data/bridge.log` |
| Lost samples at each stream stop | 0 (5 stops) | `data/bridge.log` |

## Files

- [notes.md](notes.md): original notes.
- `data/bridge.log`: the bridge's log. `scripts/client.py`: the test client. See [data/DATA.md](data/DATA.md).
- Not kept: the client's console output (the rate numbers are only in notes.md).

## How to re-run

With the board connected, run `esp32sdr bridge --listen 127.0.0.1:1234 --freq 2402 --dec 512 --bits 8`, then `host/.venv/bin/python experiments/04-rtltcp-bridge/scripts/client.py`.

## Ran

2026-11-05 13:37 EST (`client.py` 13:37:04, `bridge.log` 13:37:52). Commit 3d79a74, results in 6a6510d (15:18).

[← 03-iq-stream](../03-iq-stream/README.md) · [index](../README.md) · [05-live-view-sdrpp →](../05-live-view-sdrpp/README.md)
