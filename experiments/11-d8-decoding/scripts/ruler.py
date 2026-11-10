import os, pathlib; HERE = pathlib.Path(__file__).resolve().parent; DATA = HERE.parent / "data" / "centred-decode"; OUT = pathlib.Path(os.environ.get("OUT", HERE.parent / "figures")); OUT.mkdir(parents=True, exist_ok=True)  # experiment layout
import numpy as np
for lo,fo in [(2439,0),(2441,0),(2436,140),(2437,-140)]:
    X=np.load(f"{DATA}/cap/lo{lo}_f{fo}.npz")["x"].astype(complex); N=X.shape[1]
    X=X-X.mean(1,keepdims=True)
    S=(np.abs(np.fft.fft(X*np.hanning(N),axis=1))**2).mean(0); f=np.fft.fftfreq(N,1/16e6)
    nom=2440e6-(lo*1e6+fo*1e3)
    m=np.abs(f-nom)<300e3; i=np.flatnonzero(m)[np.argmax(S[m])]
    m2=np.abs(f+nom)<300e3; j=np.flatnonzero(m2)[np.argmax(S[m2])]
    print(lo,fo,"expect %+.1f kHz: peak at %+.1f kHz (%.1f dB over median); mirror side peak %+.1f (%.1f dB)"%(nom/1e3,f[i]/1e3,10*np.log10(S[i]/np.median(S)),f[j]/1e3,10*np.log10(S[j]/np.median(S))))
