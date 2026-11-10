import os, pathlib; HERE = pathlib.Path(__file__).resolve().parent; DATA = HERE.parent / "data" / "mode2-vs-mode0"; OUT = pathlib.Path(os.environ.get("OUT", HERE.parent / "figures")); OUT.mkdir(parents=True, exist_ok=True)  # experiment layout
import pickle, numpy as np, matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
allp,fails=pickle.load(open(os.environ.get('UNION', DATA / 'union.pkl'),'rb'))
fig,ax=plt.subplots(2,1,figsize=(12,8),sharex=True)
for a,f,title in [(ax[0],'hl_m2','mode 2 (LO 2417.033 MHz, signal found at about +151 kHz, decoded after an fs/2 shift)'),(ax[1],'hl_m0','mode 0 (LO 2421.033 MHz; LO leakage + 1-bit limiter corrupt most bits)')]:
    ks=sorted(k for k in allp if k[0]==f)
    t=np.array([k[1] for k in ks]); ch=np.array([allp[k][0] for k in ks])*2/3
    fx=np.array([allp[k][1] is not None for k in ks])
    for c,col in zip(range(8),plt.cm.tab10.colors):
        if len(t):
            a.plot(t[~fx],ch[~fx,c],'o',color=col,label=f'ch{c+1}')
            a.plot(t[fx],ch[fx,c],'x',color=col)
    a.set_title(f'{f}: {title}\n{len(t)} CRC-valid packets, tx id 66,178 (o = as received, x = one bit repaired by CRC)',fontsize=9)
    a.set_ylabel('channel [us] (raw x 2/3)'); a.set_ylim(900,2100); a.grid(alpha=.3)
ax[0].legend(ncol=8,fontsize=8,loc='upper right'); ax[1].set_xlabel('time in recording [s]')
fig.suptitle('FrSky D8 from the E8, 1-bit I/Q at 250 kS/s, stick moved slowly end to end',fontsize=11)
fig.tight_layout(); fig.savefig(OUT / 'stick_sweep.png',dpi=90)
