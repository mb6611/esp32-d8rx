import sys; sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parent))
from probe_hop import *
def an(fn, win=128, step=16, show=True):
    d=np.load(fn); caps=d['caps']; tm=d['times']; dirs=d['dirs']; a=int(d['a']); b=int(d['b']); fs=float(d['fs'])
    res=[]
    for dr in (0,1):
        src,dst=(a,b) if dr==0 else (b,a)
        sel=caps[dirs==dr]
        ln=lambda f: (2400000-f) if f<2440000 else (2480000-f)
        L1=[];L2=[];DC=[];PW=[]
        for x in sel:
            c,l1,dc,pw=settle_profile(x,ln(src),fs,win,step); _,l2,_,_=settle_profile(x,ln(dst),fs,win,step)
            L1.append(l1);L2.append(l2);DC.append(np.abs(dc));PW.append(pw)
        l1=10*np.log10(np.median(L1,0));l2=10*np.log10(np.median(L2,0));dcm=np.median(DC,0);pwm=10*np.log10(np.median(PW,0))
        t0=np.median(tm[dirs==dr,0]);t1=np.median(tm[dirs==dr,1])
        fin=np.median(l2[c>c[-1]-150]); ini=np.median(l1[c<t0-10])
        bad=np.where(np.abs(l2-fin)>3)[0]; arr=c[bad[-1]+1] if len(bad) else c[0]
        gone=c[np.argmax((c>t0)&(l1<ini-10))]
        print(f"{fn} {src}->{dst} t0={t0:.1f} t1={t1:.1f} src_line_gone@{gone:.0f}us dst_line_within3dB@{arr:.0f}us (={arr-t1:+.0f} vs t1) final={fin:.1f} ini={ini:.1f}")
        if show:
            for us in np.arange(t0-20, min(t1+120,c[-1]), 10 if t1-t0<300 else 20):
                i=np.argmin(abs(c-us))
                print(f"   t={c[i]:6.1f} src={l1[i]:5.1f} dst={l2[i]:5.1f} |dc|={dcm[i]:6.1f} pwr={pwm[i]:5.1f}")
for fn in sys.argv[1:]: an(fn, show='-q' not in fn)
