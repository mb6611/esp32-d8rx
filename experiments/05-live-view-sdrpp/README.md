# 05 · Live view in gqrx and SDR++, and a bridge fix

After the scripted check in [04](../04-rtltcp-bridge/README.md) I wanted a live waterfall in a real viewer that I could just leave running. The first try with gqrx, at 2480 MHz, ended with the bridge dropping the client: the log ends in `IQS stream failed: short read 1006/1028`, meaning the bridge had read 1006 bytes of a 1028-byte frame and then gave up.

First I wanted to know whether the bridge failed with any client or only with gqrx. A scripted client that copies gqrx's connect sequence and retunes, but reads the socket all the time, ran for 90 s at 28 to 32 kS/s per second, with 0 lost samples at each of its 14 tunes. gqrx only tunes what a real RTL-SDR can (up to 1766 MHz), so the bridge adds a fixed offset to every requested frequency; for the scripted clients it was 1000 MHz, so the client's 1480 MHz meant 2480 on the board. Then I ran the bridge with gqrx again, turned gqrx's DSP off and quit gqrx, and got the same failure, `short read 1027/1028`.

The cause was in the bridge. While gqrx's DSP is off, gqrx stops reading the socket. The bridge wrote to the socket with a blocking `sendall`, so it blocked too, stopped reading the serial port, and the USB-serial buffer overflowed and dropped bytes. A frame cut short by the missing bytes then took the following bytes as its own. When the stream ended for a retune, that frame's payload read swallowed the `IQSEND` line and waited for bytes that never came, the transport discarded what it had, and the bridge dropped the client.

I changed three things. A separate sender thread now does the TCP writes from a queue of 64 blocks, so the serial reader never waits on the client; when the queue is full the bridge drops display blocks and counts them ("blocks dropped for a slow client"), and a 64 KiB socket send buffer bounds how stale the backlog can get. The stream reader no longer loses bytes on a timeout; once a frame has started it waits only 0.5 s for the rest, then marks the frame as cut short, slips one byte and searches for the next frame or the `IQSEND` line. And a stream run that fails is logged with its loss and restarted; only 5 runs in a row with no data end the client. The bridge also prints a status line every 30 s with samples, lost samples, short frames, bad CRCs and dropped blocks.

To check the fix I used a client that behaves like gqrx with its DSP off: it connects, stops reading for 40 s, then retunes to 2480 MHz (already tuned, so no restart) and, 30 s later, to 2402 MHz. The bridge kept the stream running throughout: 0 lost samples, 0 short frames and 0 bad CRCs, and 1735 blocks dropped while the client wasn't reading (1126 of them by the first 30 s status line). Once the client read again the count stayed at 1735, and the rate was back to 31.25 kS/s after the backlog flushed.

For SDR++ I added a launcher, `esp32sdr sdrpp`. SDR++'s rtl_tcp source won't go below 250 kS/s, so the bridge interpolates the 31.25 kS/s stream by 8 and SDR++ decimates by 8 back to a 31.25 kHz span. The launcher backs up the SDR++ config, sets it up, starts SDR++ and presses play through its rigctl server. After two short tries at 13:58 and 13:59, I left one session running from 14:01 to 14:05: over 2.5 minutes, at 2480 MHz with a rigctl retune to 2402 MHz (`F 2402000000`) and back, and six small offset changes of 5 to 12 kHz. Every 30 s status line reported 936,448 samples (31,215 S/s) with 0 lost, 0 short frames, 0 bad CRCs and 0 dropped blocks, so SDR++ kept up the whole time.

So the live view holds up even when the viewer stops reading, and SDR++ became the viewer I use. The test covered one slow client and one SDR++ session of a few minutes, not hours. How to use it is in [docs/devkitv1/live-view.md](../../docs/devkitv1/live-view.md). With a working live view, I tried it on a Bluetooth signal I could switch on and off ([06](../06-ble-advertising/README.md)).

---

## Numbers

| What | Value | Where |
|---|---|---|
| Failure before the fix | `short read 1006/1028` (gqrx), `1027/1028` (reproduced) | `data/bridge2480.log`, `data/bridge2.log` |
| Stall client, 40 s not reading: lost samples / dropped blocks | 0 / 1735 | `data/bridge3.log` |
| SDR++ session: samples per 30 s, lost, dropped | 936,448 (31,215 S/s), 0, 0 | `data/sdrpp_run3.log` |

## Files

- Notes: the "Bridge robustness" section of [docs/devkitv1/live-view.md](../../docs/devkitv1/live-view.md) (kept there because it is also the usage doc).
- `data/`: bridge, client and SDR++ launcher logs, in time order: `bridge2480.log` (the failure with gqrx, 13:47), `bridge1.log` and `client1.log` (scripted gqrx-like client, 13:51), `bridge2.log` (failure reproduced, 13:52), `sdrpp_run.log`, `sdrpp_run2.log` (13:59), `bridge3.log` (stall test after the fix, 14:01), `sdrpp_run3.log` (the long SDR++ session, 14:05). See [data/DATA.md](data/DATA.md).
- `scripts/gqrx_like.py`, `scripts/stall_client.py`: the scripted clients (need the board and a running bridge).

## How to re-run

With the board connected, run `esp32sdr bridge --freq 2480 --offset-mhz 1000`, then `host/.venv/bin/python experiments/05-live-view-sdrpp/scripts/stall_client.py`; or `esp32sdr sdrpp`.

## Ran

2026-11-05 13:47–14:05 EST (`bridge2480.log` 13:47, start 13:48, `bridge3.log` 14:01, `sdrpp_run3.log` 14:05). Commit 3d79a74 (15:18); docs in 707ca28 (18:49).

[← 04-rtltcp-bridge](../04-rtltcp-bridge/README.md) · [index](../README.md) · [06-ble-advertising →](../06-ble-advertising/README.md)
