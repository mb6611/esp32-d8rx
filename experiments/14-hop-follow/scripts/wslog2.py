import asyncio,json,time,sys,websockets
DUR=float(sys.argv[1]) if len(sys.argv)>1 else 30
OUT=sys.argv[2] if len(sys.argv)>2 else "/tmp/d8/follow_ws.json"
async def main():
    out=[]; t0=time.time()
    async with websockets.connect("ws://127.0.0.1:8765/ws") as ws:
        while time.time()-t0<DUR:
            try: m=json.loads(await asyncio.wait_for(ws.recv(),1))
            except asyncio.TimeoutError: continue
            out.append({"t":time.time()-t0,"m":m})
    json.dump(out,open(OUT,"w"))
    print(len(out),"msgs,",sum(1 for o in out if o["m"].get("type")=="packet" and o["m"].get("crc_ok")),"crc-ok packets")
asyncio.run(main())
