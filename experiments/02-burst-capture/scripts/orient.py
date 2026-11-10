import sys as _sys, pathlib as _pl; _sys.path.insert(0, str(_pl.Path(__file__).resolve().parents[3] / "host"))  # repo host/ package
import sys, numpy as np, time
from esp32sdr.device import Device
from esp32sdr.samples import decode_cap
from esp32sdr.spectrum import psd
from esp32sdr.spec_cmd import spectrum
d=Device.open()
def band(f,p,lo,hi):
    m=(f>=lo)&(f<=hi); return 10*np.log10(np.mean(10**(p[m]/10)))
for lo_mhz, label in [(2402,"ch1-10"),(2422,"ch1+10"),(2427,"ch6-10"),(2447,"ch6+10"),(2452,"ch11-10"),(2472,"ch11+10")]:
    d.tune(lo_mhz)
    diffs=[]
    for k in range(40):
        iq=decode_cap(d.cap16(16380,1),8)
        f,p=psd(iq,40e6,256)
        up=band(f,p,3e6,17e6); dn=band(f,p,-17e6,-3e6)
        diffs.append(up-dn)
    diffs=np.array(diffs)
    r=spectrum(d,1024,1,2000,detector=1)
    fs,ps=r.sorted_db(); fb=fs-lo_mhz*1e6
    sup=band(fb,ps,3e6,17e6); sdn=band(fb,ps,-17e6,-3e6)
    print(f"{label} LO {lo_mhz}: capture upper-lower dB median {np.median(diffs):+.1f} max {diffs.max():+.1f} min {diffs.min():+.1f} n(>+6)={int((diffs>6).sum())} n(<-6)={int((diffs<-6).sum())} | SPEC max-hold (LO-f mapping) upper-lower {sup-sdn:+.1f}", flush=True)
d.close()
