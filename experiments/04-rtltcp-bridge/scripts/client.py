import socket,struct,time,sys
def rd(s,sec):
    s.settimeout(1.0); n=0; t=time.time(); buf=b""
    while time.time()-t<sec:
        try: d=s.recv(65536)
        except socket.timeout: continue
        if not d: print("EOF"); break
        n+=len(d); buf+=d[-4000:]
    dt=time.time()-t
    return n/dt/2, buf
for rnd in range(2):
    s=socket.create_connection(("127.0.0.1",1234)); s.settimeout(10)
    h=b""
    while len(h)<12: h+=s.recv(12-len(h))
    print("hdr",h, struct.unpack(">4sII",h))
    r,b=rd(s,10); print("rate S/s",r)
    if rnd==0:
      for f in (2400,2483,2426):
        s.sendall(struct.pack(">BI",1,f*1000000)); time.sleep(1.5)
        r,b=rd(s,6); iq=list(b[-200:]); print(f,"rate",r,"mean",sum(iq)/len(iq))
    s.close(); time.sleep(2)
