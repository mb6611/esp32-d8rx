import os, pathlib; HERE = pathlib.Path(__file__).resolve().parent; DATA = HERE.parent / "data" / "iq-orientation"; OUT = pathlib.Path(os.environ.get("OUT", HERE.parent / "figures")); OUT.mkdir(parents=True, exist_ok=True)  # experiment layout
import numpy as np
z=np.load(DATA / "sweep.npz"); los=z["los"]; fb=z["fb"]; fs=40e6; NB=len(fb)
grid=np.arange(2380e6,2500e6,fs/NB)
def rows(S,s):
    S=S-np.median(S,axis=0); m=np.abs(fb)<16e6; R=[]
    for i,lo in enumerate(los):
        rf=lo*1e6+s*fb[m]; o=np.argsort(rf)
        R.append(np.interp(grid,rf[o],S[i][m][o],left=np.nan,right=np.nan))
    return np.array(R)
for key in ("cap","spc"):
    A=rows(z[key],+1); B=rows(z[key],-1); win=0; tot=0
    for i in range(len(los)):
        for j in range(i+1,min(i+4,len(los))):
            k=~np.isnan(A[i])&~np.isnan(A[j]); kb=~np.isnan(B[i])&~np.isnan(B[j])
            ca=np.corrcoef(A[i][k],A[j][k])[0,1]; cb=np.corrcoef(B[i][kb],B[j][kb])[0,1]
            tot+=1; win+= ca>cb
    print(key, f"LO pairs where RF=LO+f fits better: {win}/{tot}")
