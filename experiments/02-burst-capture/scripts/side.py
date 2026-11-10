import sys as _sys, pathlib as _pl; _sys.path.insert(0, str(_pl.Path(__file__).resolve().parents[3] / "host"))  # repo host/ package
import numpy as np
from esp32sdr.device import Device
from esp32sdr.samples import decode_cap
from esp32sdr.spectrum import psd
d=Device.open(); fs=40e6
def bandlin(f,p,lo,hi):
    m=(f>=lo)&(f<=hi); return np.mean(10**(p[m]/10))
for lo,expect in ((2452,"upper (+10)"),(2472,"lower (-10)"),(2427,"upper (+10, ch6)")):
    d.tune(lo); P=[]
    for k in range(120):
        f,p=psd(decode_cap(d.cap16(16380,1),8),fs,256); P.append(p)
    P=np.array(P); base=np.median(P,axis=0)
    up=[];dn=[]
    for p in P:
        u=10*np.log10(bandlin(f,p,5e6,15e6)/bandlin(f,base,5e6,15e6))
        l=10*np.log10(bandlin(f,p,-15e6,-5e6)/bandlin(f,base,-15e6,-5e6))
        up.append(u); dn.append(l)
    up=np.array(up); dn=np.array(dn)
    bu=int(((up>6)&(up>dn+3)).sum()); bl=int(((dn>6)&(dn>up+3)).sum())
    print(f"LO {lo} (expect {expect}): bursts >6 dB over typical, dominant upper +5..+15 MHz: {bu}, dominant lower -15..-5 MHz: {bl}  (of 120)", flush=True)
d.close()
