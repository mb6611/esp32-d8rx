import sys,json,numpy as np
sys.path.insert(0, str(__import__("pathlib").Path(__file__).resolve().parents[3] / "host"))
from esp32sdr.device import Device
from esp32sdr.iqs import IqsSession
tag,mhz,fofs,mode,secs=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),int(sys.argv[4]),float(sys.argv[5])
dev=Device.open("/dev/cu.usbserial-0001")
s=IqsSession(dev,64,1,mhz,fofs,seconds=secs,mode=mode)
x=np.concatenate([iq for iq,fr in s.blocks()]).astype(np.complex64)
x.tofile(f"/tmp/d8/{tag}.sigmf-data")
json.dump({"global":{"core:datatype":"cf32_le","core:sample_rate":250000.0,"core:version":"1.0.0","esp32sdr:bits":1,"esp32sdr:decimation":64,"esp32sdr:mode":mode,"esp32sdr:lo_mhz":mhz,"esp32sdr:fofs_khz":fofs},"captures":[{"core:sample_start":0,"core:frequency":(mhz+fofs/1000+(4 if mode==2 else 0))*1e6}],"annotations":[]},open(f"/tmp/d8/{tag}.sigmf-meta","w"))
st=s.stats; print(tag,"samples",len(x),"gap",st.gap_samples,"badcrc",st.bad_crc,"Imean %.2f Qmean %.2f"%(x.real.mean(),x.imag.mean()))
