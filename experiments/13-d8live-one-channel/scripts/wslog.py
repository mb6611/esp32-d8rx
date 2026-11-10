import asyncio,json,time,websockets
async def main():
    out=[]; t0=time.time()
    async with websockets.connect("ws://127.0.0.1:8765/ws") as ws:
        while time.time()-t0<30:
            try: m=json.loads(await asyncio.wait_for(ws.recv(),1))
            except asyncio.TimeoutError: continue
            if m.get("type")=="packet" and m.get("crc_ok"): out.append({"t":time.time()-t0,"ch":m["ch"]})
    json.dump(out,open("/tmp/d8/sticks.json","w")); print(len(out),"packets")
asyncio.run(main())
