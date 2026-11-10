"""Scripted rtl_tcp client mimicking gqrx's connect sequence + retunes."""
import socket, struct, sys, time, threading

host, port = "127.0.0.1", 1234
dur = float(sys.argv[1]) if len(sys.argv) > 1 else 60
s = socket.create_connection((host, port))
hdr = s.recv(12)
print("header", hdr)
def cmd(c, a):
    s.sendall(struct.pack(">BI", c, a & 0xFFFFFFFF))
total = [0]
stop = [False]
def rx():
    while not stop[0]:
        try:
            d = s.recv(65536)
        except OSError:
            break
        if not d:
            print("EOF from bridge"); break
        total[0] += len(d)
threading.Thread(target=rx, daemon=True).start()
# gqrx-ish init
cmd(0x02, 31250); cmd(0x05, 0); cmd(0x03, 1); cmd(0x08, 0); cmd(0x09, 0); cmd(0x0a, 0)
cmd(0x01, 1024_000_000); cmd(0x04, 0); cmd(0x01, 1480_000_000)
t0 = time.time(); last = 0; i = 0
seq = [1480_000_000, 1480_000_000, 1402_000_000, 1480_000_000, 1480_010_000, 1024_000_000]
while time.time() - t0 < dur:
    time.sleep(1)
    rate = (total[0] - last) / 2; last = total[0]
    print(f"{time.time()-t0:5.0f}s  {rate:8.0f} S/s", flush=True)
    if int(time.time() - t0) % 7 == 0:
        f = seq[i % len(seq)]; i += 1
        cmd(0x01, f); cmd(0x02, 31250); cmd(0x04, 100)
stop[0] = True
s.close()
