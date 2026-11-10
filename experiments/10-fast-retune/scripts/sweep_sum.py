import sys; sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parent))
from probe_hop import *
fn=sys.argv[1]; d=np.load(fn); fs=RATE_HZ[int(d['rate'])] if 'rate' in d else 40e6
rows=[]
for khz in D8_KHZ:
    if f"fast_{khz}" not in d: continue
    fa,fu=list(d[f"fast_{khz}"]),list(d[f"full_{khz}"])
    qf=[quiet_stats(x) for x in fa]; qr=[quiet_stats(x) for x in fu]
    rf=ruler_snr(fa,khz,fs); rr=ruler_snr(fu,khz,fs)
    bin_khz=fs/16380/1e3
    best=lambda r: max(r,key=lambda t:t[2]) if r else None
    bf,br=best(rf),best(rr)
    rows.append(dict(khz=khz,dcf=np.median([q['dc_abs'] for q in qf]),dcr=np.median([q['dc_abs'] for q in qr]),
        nf=np.median([q['noise_rms'] for q in qf]),nr=np.median([q['noise_rms'] for q in qr]),
        cf=np.mean([q['clip'] for q in qf]),cr=np.mean([q['clip'] for q in qr]),
        rerr=(bf[1]-bf[0]) if bf else np.nan, rsnr=bf[2] if bf else np.nan, rsnr_full=br[2] if br else np.nan,bin=bin_khz))
import json
for r in rows: print(f"{r['khz']/1e3:9.3f} dc {r['dcf']:5.1f}/{r['dcr']:5.1f} noise {r['nf']:6.1f}/{r['nr']:6.1f} clip {r['cf']:.3f}/{r['cr']:.3f} ruler err {r['rerr']:+6.1f} kHz snr {r['rsnr']:5.1f}/{r['rsnr_full']:5.1f}")
ok=[r for r in rows if r['rsnr']>=8]
print("channels",len(rows),"ruler>=8dB",len(ok),"max |err| kHz",max(abs(r['rerr']) for r in ok) if ok else None,"bin",rows[0]['bin'])
q=[r for r in rows if r['nf']<15 and r['nr']<15]
print("quiet-pair channels",len(q),"dc fast med/max",np.median([r['dcf'] for r in q]) if q else None,max([r['dcf'] for r in q]) if q else None,"dc full med/max",np.median([r['dcr'] for r in q]) if q else None,max([r['dcr'] for r in q]) if q else None)
print("clip fast mean",np.mean([r['cf'] for r in rows]),"full mean",np.mean([r['cr'] for r in rows]))
