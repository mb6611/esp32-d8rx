import sys as _sys, pathlib as _pl; _sys.path.insert(0, str(_pl.Path(__file__).resolve().parents[3] / "host"))  # repo host/ package
import time
from esp32sdr.device import Device, TransferError
d=Device.open(); d.tune(2437)
for bits in (8,10):
    for code,name in ((0,80),(1,40),(6,16)):
        bad=0; t=time.time()
        for i in range(50):
            try: d.cap16(16380,code,bits)
            except TransferError as e: bad+=1; print(" ",e)
        print(f"{name} MS/s {bits}-bit: 50 captures, {bad} failures, {(time.time()-t)/50*1000:.0f} ms each", flush=True)
print("after:", d.info())
d.close()
