import os, pathlib; HERE = pathlib.Path(__file__).resolve().parent; DATA = HERE.parent / "data" / "iq-orientation"; OUT = pathlib.Path(os.environ.get("OUT", HERE.parent / "figures")); OUT.mkdir(parents=True, exist_ok=True)  # experiment layout
import numpy as np, matplotlib; matplotlib.use("Agg"); import matplotlib.pyplot as plt
z=np.load(DATA / "sweep.npz"); los=z["los"]; fb=z["fb"]
fig,axs=plt.subplots(2,2,figsize=(12,6),sharey=True)
for col,(key,lab,sgood) in enumerate((("cap","host capture PSD (decode_cap, Q negated)",+1),("spc","on-chip SPEC bins",-1))):
    S=z[key]-np.median(z[key],axis=0)
    for row,s in enumerate((+1,-1)):
        ax=axs[row,col]
        for i,lo in enumerate(los):
            m=np.abs(fb)<16e6
            rf=(lo*1e6+s*fb[m])/1e6; o=np.argsort(rf)
            ax.scatter(rf[o],np.full(m.sum(),lo),c=S[i][m][o],s=4,marker="s",cmap="viridis",vmin=-3,vmax=15)
        ax.set_title(f"{lab}: RF = LO {'+' if s>0 else '-'} f{'  (consistent)' if s==sgood else ''}",fontsize=8)
        ax.set_xlabel("RF MHz"); ax.set_ylabel("LO MHz")
fig.tight_layout(); fig.savefig(OUT / "orientation-sweep.png",dpi=100)
# RF activity under host mapping
S=z["cap"]; grid=np.arange(2395,2490,0.5); acc=np.full((len(los),grid.size),np.nan)
for i,lo in enumerate(los):
    m=np.abs(fb)<16e6; rf=(lo*1e6+fb[m])/1e6
    acc[i]=np.interp(grid,rf,(S[i]-np.median(S,axis=0))[m],left=np.nan,right=np.nan)
a=np.nanmean(acc,axis=0)
for ch,f in ((1,2412),(6,2437),(11,2462)):
    k=np.abs(grid-f)<8; print(ch, round(float(np.nanmean(a[k])),2))
