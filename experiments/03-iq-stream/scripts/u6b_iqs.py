#!/usr/bin/env python3
"""U6b bench check: run IQS, parse IQS1 frames, verify CRC/sequence/index, print IQSEND."""
import struct, sys, time, zlib
import serial

port = "/dev/cu.usbserial-0001"
args = sys.argv[1:]
freq = int(args[0]) if args else 2402
cmd = args[1] if len(args) > 1 else "IQS 60000 512 8 6"
reset = "--reset" in args

s = serial.Serial(port, 921600, timeout=0.2)
if reset:
    s.rts = True; s.dtr = False; time.sleep(0.1); s.rts = False; time.sleep(1.5)
s.reset_input_buffer()

def line(timeout=5.0):
    t = time.time(); b = b""
    while time.time() - t < timeout:
        c = s.read(1)
        if not c: continue
        if c == b"\n": return b.decode(errors="replace")
        b += c
    return b.decode(errors="replace") + "<timeout>"

def sync():
    for n in range(20):
        s.write(b"RELEASE\n"); time.sleep(0.05); s.reset_input_buffer()
        s.write(f"SYNC {n}\n".encode())
        t = time.time()
        while time.time() - t < 1:
            l = line(1)
            if l.strip() == f"SYNC {n}": return True
    return False

print("sync", sync())
for c in ["VERSION?", "CAPS", f"FREQ {freq}"]:
    s.write((c + "\n").encode()); print(">", c, "|", line())
s.write((cmd + "\n").encode())
print(">", cmd, "|", line())
buf = b""; frames = 0; bad_crc = 0; seq_err = 0; idx_jumps = 0; jump_samples = 0; flags_seen = {}
exp_seq = 0; exp_idx = None; samples = 0; end = None; t0 = time.time(); first_idx = None; nbytes = 0
s.timeout = 0.05
while True:
    d = s.read(8192)
    nbytes += len(d)
    buf += d
    while True:
        if len(buf) < 4: break
        if buf[:4] != b"IQS1":
            k = buf.find(b"IQS1"); n = buf.find(b"IQSEND")
            if n >= 0 and (k < 0 or n < k):
                e = buf.find(b"\n", n)
                if e < 0: break
                end = buf[n:e].decode(); buf = buf[e + 1:]; break
            if k < 0: buf = buf[-8:]; break
            print("skip", buf[:k][:60]); buf = buf[k:]; continue
        if len(buf) < 24: break
        magic, seq, idx, ns, bits, flags, dec, gain, shift = struct.unpack("<IIQHBBHBB", buf[:24])
        per = {4: 1, 8: 2, 16: 4}[bits]
        L = 24 + ns * per + 4
        if len(buf) < L: break
        fr = buf[:L]; buf = buf[L:]
        crc = struct.unpack("<I", fr[-4:])[0]
        if zlib.crc32(fr[:-4]) != crc: bad_crc += 1; continue
        frames += 1
        if seq != exp_seq: seq_err += 1
        exp_seq = seq + 1
        if first_idx is None: first_idx = idx
        if exp_idx is not None and idx != exp_idx:
            idx_jumps += 1; jump_samples += idx - exp_idx
            print(f"jump at frame {seq}: {exp_idx} -> {idx} flags {flags}")
        exp_idx = idx + ns; samples += ns
        flags_seen[flags] = flags_seen.get(flags, 0) + 1
    if end: break
    if time.time() - t0 > 120: print("timeout"); break
el = time.time() - t0
print(f"frames={frames} bad_crc={bad_crc} seq_err={seq_err} samples={samples} jumps={idx_jumps} jump_samples={jump_samples} flags={flags_seen} first_idx={first_idx} wall={el:.1f}s rate={samples/el:.0f}/s bytes={nbytes}")
print(end)
if end:
    f = end.split()
    pairs, lost = int(f[4]), int(f[10])
    print(f"IQSEND pairs={pairs} lost_pairs={lost} lost_frac={lost/max(1,pairs+lost)*100:.5f}% units={f[3]} bad_sessions={f[2]} late_max={f[6]} work_max={f[7]}")
    span = (exp_idx - first_idx) if first_idx is not None else 0
    print(f"index span {span}, delivered {samples}, index loss frac {(span-samples)/max(1,span)*100:.5f}%")
s.write(b"INFO\n"); print("> INFO |", line())
