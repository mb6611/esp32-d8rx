import sys as _sys, pathlib as _pl; _sys.path.insert(0, str(_pl.Path(__file__).resolve().parents[3] / "host"))  # repo host/ package
import os, pathlib; HERE = pathlib.Path(__file__).resolve().parent; DATA = HERE.parent / "data" / "mode2-vs-mode0"; OUT = pathlib.Path(os.environ.get("OUT", HERE.parent / "figures")); OUT.mkdir(parents=True, exist_ok=True)  # experiment layout
import numpy as np, itertools, pickle
from esp32sdr import d8
fs=250000.
allp={}
fails=[]
for f,shifts,notches in [('hl_m2',[125e3,120e3,130e3],[0]),('hl_m0',[0],[0,64])]:
    x=np.fromfile(f'{DATA}/{f}.sigmf-data',dtype=np.complex64)
    for sh,nt,cut,dump in itertools.product(shifts,notches,[60e3,75e3,90e3],[0.3,0.6]):
        r=d8.StandardDecoder(fs,shift_hz=sh,dc_notch=nt,cutoff=cut,dump=dump).decode(x,fix1=True)
        for q in r:
            p=q.packet
            if p.tx_id!=(66,178): continue
            key=(f,round(q.t,3))
            if p.crc_ok and p.raw[3]==0 and p.raw[5]==1:
                if key not in allp or (allp[key][1] is not None and q.fixed_bit is None):
                    allp[key]=(p.channels,q.fixed_bit,p.raw[4])
            else: fails.append((f,q.t,p.channels))
    print(f,'cum unique crc-valid',sum(1 for k in allp if k[0]==f))
pickle.dump((allp,fails),open(OUT / 'union.pkl','wb'))
for k in sorted(allp): print(k,allp[k])
