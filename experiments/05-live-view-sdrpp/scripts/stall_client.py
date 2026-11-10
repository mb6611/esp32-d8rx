"""rtl_tcp client that mimics gqrx with DSP off: connects, stops reading for a while,
then resumes and retunes (offset 1000 MHz convention), and checks data keeps flowing."""
import socket, struct, sys, time

s = socket.create_connection(("127.0.0.1", 1234))
s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536)
def cmd(c, a): s.sendall(struct.pack(">BI", c, a))
print("header", s.recv(12), flush=True)
cmd(0x02, 31250); cmd(0x01, 24_000_000); cmd(0x01, 480_000_000)
print("not reading for 40 s", flush=True)
time.sleep(40)
cmd(0x01, 1480_000_000)  # user types 2480 (hw 1480 -> bridge 2480)
t0 = time.time(); n = 0; last = t0; per = 0
s.settimeout(10)
while time.time() - t0 < 60:
    d = s.recv(65536)
    if not d:
        print("EOF: bridge ended the client", flush=True); sys.exit(1)
    n += len(d); per += len(d)
    if time.time() - last >= 10:
        print(f"{time.time()-t0:5.1f}s {per/2/(time.time()-last):8.0f} S/s", flush=True)
        last = time.time(); per = 0
    if 29.9 < time.time() - t0 < 30.1:
        cmd(0x01, 1402_000_000)
print(f"OK: {n/2:.0f} samples in 60 s after the stall", flush=True)
