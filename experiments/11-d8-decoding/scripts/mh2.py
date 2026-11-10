import os, pathlib; HERE = pathlib.Path(__file__).resolve().parent; DATA = HERE.parent / "data" / "centred-decode"; OUT = pathlib.Path(os.environ.get("OUT", HERE.parent / "figures")); OUT.mkdir(parents=True, exist_ok=True)  # experiment layout
import numpy as np,sys,json
for tag in sys.argv[1:]:
    x=np.fromfile(f"{DATA}/{tag}.sigmf-data",dtype="<c8"); fs=250e3
    g=json.load(open(f"{DATA}/{tag}.sigmf-meta"))["global"]
    N=256; n=len(x)//N
    X=np.abs(np.fft.fftshift(np.fft.fft(x[:n*N].reshape(n,N)*np.hanning(N),axis=1),axes=1))**2
    f=np.fft.fftshift(np.fft.fftfreq(N,1/fs))/1e3
    med=np.median(X,0)
    # frames with burst: high peak ratio
    pk=X.max(1)/np.median(X,1)
    thr=np.percentile(pk,97)
    sel=X[pk>thr]
    avg=sel.mean(0)/med
    top=np.argsort(avg)[::-1][:8]
    print(tag,g.get("esp32sdr:mode"),g.get("esp32sdr:fofs_khz"),"burstframes",len(sel),"mean",x.mean())
    print("  top bins kHz:",[(round(f[i],1),round(10*np.log10(avg[i]),1)) for i in sorted(top)])
