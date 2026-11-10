import sys as _sys, pathlib as _pl; _sys.path.insert(0, str(_pl.Path(__file__).resolve().parents[3] / "host"))  # repo host/ package
import numpy as np
from esp32sdr.device import Device
from esp32sdr.samples import decode_cap
from esp32sdr.spectrum import psd
d=Device.open(); fs=40e6
for ch,fc in ((6,2437),(11,2462)):
    lo=fc-10; d.tune(lo)
    P=[]
    for k in range(80):
        f,p=psd(decode_cap(d.cap16(16380,1),8),fs,256); P.append(p)
    P=np.array(P); base=np.median(P,axis=0); ex=P-base
    cents=[]
    for e in ex:
        m=e>10   # bins at least 10 dB above the typical level
        m &= np.abs(f)>0.5e6
        if m.sum()>=15:  # wide burst (>=2.3 MHz occupied)
            w=10**(e[m]/10); cents.append(np.sum(f[m]*w)/w.sum()/1e6)
    c=np.array(cents)
    h,edges=np.histogram(c,bins=np.arange(-20,21,4))
    print(f"ch{ch} ({fc} MHz), LO {lo}: {len(c)}/80 captures with a wide burst; centroid MHz median {np.median(c) if len(c) else float('nan'):+.1f}; "
          f"in +6..+14: {int(((c>6)&(c<14)).sum())}, in -14..-6: {int(((c>-14)&(c<-6)).sum())}; hist {dict(zip(edges[:-1].astype(int).tolist(),h.tolist()))}", flush=True)
d.close()
