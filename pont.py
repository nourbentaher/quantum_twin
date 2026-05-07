import asyncio
import websockets

TCP_HOST = "127.0.0.1"
TCP_PORT = 9090

async def handler(websocket):
    print(f"[PONT] Navigateur connecté")
    try:
        reader, writer = await asyncio.open_connection(TCP_HOST, TCP_PORT)
    except Exception as e:
        print(f"[PONT] Impossible de rejoindre le serveur TCP : {e}")
        return

    async def tcp_to_ws():
        try:
            while True:
                data = await reader.read(4096)
                if not data:
                    break
                await websocket.send(data.decode("utf-8", errors="replace"))
        except Exception:
            pass

    async def ws_to_tcp():
        try:
            async for message in websocket:
                writer.write(message.encode("utf-8"))
                await writer.drain()
        except Exception:
            pass

    await asyncio.gather(tcp_to_ws(), ws_to_tcp())
    writer.close()

async def main():
    print("[PONT] En écoute WebSocket sur ws://localhost:8080")
    async with websockets.serve(handler, "0.0.0.0", 8080):
        await asyncio.Future()

asyncio.run(main())
