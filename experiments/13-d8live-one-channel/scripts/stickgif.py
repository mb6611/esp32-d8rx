import os, pathlib; HERE = pathlib.Path(__file__).resolve().parent; DATA = HERE.parent / "data"; OUT = pathlib.Path(os.environ.get("OUT", HERE.parent / "figures")); OUT.mkdir(parents=True, exist_ok=True)  # experiment layout
import json,math
from PIL import Image,ImageDraw,ImageFont
pk=json.load(open(DATA / "sticks.json")); LO,HI=980,2020
fr=lambda u:min(1,max(0,(u-LO)/(HI-LO)))
W,H,S=560,300,2  # draw at 2x then downscale for antialiasing
FPS=15; T=pk[-1]["t"]; n=int(T*FPS)
try: font=ImageFont.truetype("/System/Library/Fonts/Menlo.ttc",13*S)
except: font=ImageFont.load_default()
pads=[("left stick  ·  yaw / throttle",3,0),("right stick  ·  roll / pitch",1,2)]
pos=[None,None]; frames=[]; i=0
for f in range(n):
    t=f/FPS
    while i+1<len(pk) and pk[i+1]["t"]<=t: i+=1
    ch=pk[i]["ch"]
    im=Image.new("RGB",(W*S,H*S),(11,14,17)); d=ImageDraw.Draw(im)
    for k,(lab,xi,yi) in enumerate(pads):
        ox=(20+k*270)*S; oy=20*S; w=250*S; cx=ox+w//2; cy=oy+w//2; R=w//2-26*S
        d.rounded_rectangle([ox,oy,ox+w,oy+w],radius=18*S,fill=(17,22,28),outline=(42,49,59),width=S)
        d.rectangle([cx-R,cy-R,cx+R,cy+R],outline=(31,39,49),width=S)
        d.line([cx,cy-R,cx,cy+R],fill=(31,39,49),width=S); d.line([cx-R,cy,cx+R,cy],fill=(31,39,49),width=S)
        d.ellipse([cx-30*S,cy-30*S,cx+30*S,cy+30*S],fill=(30,36,43),outline=(57,66,78),width=S)
        tx=cx+(fr(ch[xi])*2-1)*R; ty=cy-(fr(ch[yi])*2-1)*R
        if pos[k] is None: pos[k]=[tx,ty]
        pos[k][0]+=(tx-pos[k][0])*0.3; pos[k][1]+=(ty-pos[k][1])*0.3
        px,py=pos[k]
        d.line([cx,cy,px,py],fill=(92,103,116),width=10*S)
        d.ellipse([px-17*S+3*S,py-17*S+4*S,px+17*S+3*S,py+17*S+4*S],fill=(5,7,9))
        for r,c in ((17,(28,95,153)),(13,(77,171,247)),(6,(159,209,255))):
            o=-(17-r)*0.35*S
            d.ellipse([px-r*S+o,py-r*S+o,px+r*S+o,py+r*S+o],fill=c)
        d.text((ox+12*S,oy+w-24*S),f"{ch[xi]:.0f} / {ch[yi]:.0f} us",fill=(138,149,163),font=font)
        d.text((ox+2*S,oy+w+6*S),lab,fill=(138,149,163),font=font)
    frames.append(im.resize((W,H),Image.LANCZOS).convert("P",palette=Image.ADAPTIVE,colors=64))
frames[0].save(OUT / "sticks.gif",save_all=True,append_images=frames[1:],duration=int(1000/FPS),loop=0,optimize=True)
print(len(frames),"frames")
