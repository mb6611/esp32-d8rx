#!/usr/bin/env python3
"""Switch the ESP32 advertiser for the A/B runs through the USB adapter's RTS line (wired to EN).
`off S` holds EN low (chip in reset, radio off) for S s; `on S` releases it so the chip boots and
advertises for S s, then puts it back in reset. Port: $ESP_PORT (default /dev/cu.usbserial-0001).
Use the ESP32's port only, never the flight controller's."""
import os, sys, time, serial
mode, secs = sys.argv[1], float(sys.argv[2])
s = serial.Serial(os.environ.get("ESP_PORT", "/dev/cu.usbserial-0001"), 115200)
s.dtr = False
if mode == "off":
    s.rts = True            # EN low: chip held in reset, radio off
else:
    s.rts = True; time.sleep(0.1); s.rts = False   # release -> boot, advertise
print(mode, "start", time.time(), flush=True)
time.sleep(secs)
print(mode, "end", time.time(), flush=True)
if mode == "on":
    s.rts = True            # leave in reset afterwards
    time.sleep(0.2)
