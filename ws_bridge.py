#!/usr/bin/env python3
"""
QUANTUM TWIN — WebSocket Bridge
Lancer dans Termux : python3 ws_bridge.py [ip_serveur] [port_serveur]
Exemple            : python3 ws_bridge.py 192.168.1.42 9090

Installation dep   : pip install websockets
"""
import asyncio
import socket
import threading
import sys

try:
    import websockets
except ImportError:
    print("Installation de websockets...")
    import subprocess
    subprocess.run([sys.executable, "-m", "pip", "install", "websockets"], check=True)
    import websockets

SERVER_IP   = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.42"
SERVER_PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 9090
BRIDGE_PORT = 8765  # port WebSocket local sur le telephone

print(f"╔══════════════════════════════════════╗")
print(f"║   QUANTUM TWIN — WS Bridge           ║")
print(f"║   Serveur C  : {SERVER_IP}:{SERVER_PORT:<16}║")
print(f"║   Bridge WS  : ws://localhost:{BRIDGE_PORT}  ║")
print(f"╚══════════════════════════════════════╝")
print(f"\nOuvre Chrome et va sur :")
print(f"  file:///sdcard/quantum_twin/client_mobile.html")
print(f"  (ou copie client_mobile.html dans le stockage)\n")

async def handle_client(websocket):
    """Un client WebSocket = une connexion TCP vers le serveur C"""
    print(f"[Bridge] Mobile connecte")
    
    # Ouvrir socket TCP vers le serveur C
    try:
        tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp.connect((SERVER_IP, SERVER_PORT))
        tcp.setblocking(False)
    except Exception as e:
        await websocket.send(f"ERROR Impossible de joindre le serveur : {e}\n")
        return

    loop = asyncio.get_event_loop()

    async def tcp_to_ws():
        """Lit le serveur C et envoie au mobile"""
        while True:
            try:
                data = await loop.sock_recv(tcp, 512)
                if not data:
                    break
                msg = data.decode("utf-8", errors="replace")
                print(f"[Serveur→Mobile] {msg.strip()}")
                await websocket.send(msg)
            except Exception:
                break

    async def ws_to_tcp():
        """Lit le mobile et envoie au serveur C"""
        try:
            async for msg in websocket:
                line = msg.strip() + "\n"
                print(f"[Mobile→Serveur] {line.strip()}")
                await loop.sock_sendall(tcp, line.encode())
        except Exception:
            pass

    # Les deux directions en parallele
    done, pending = await asyncio.wait(
        [asyncio.create_task(tcp_to_ws()),
         asyncio.create_task(ws_to_tcp())],
        return_when=asyncio.FIRST_COMPLETED
    )
    for t in pending:
        t.cancel()

    tcp.close()
    print(f"[Bridge] Mobile deconnecte")

async def main():
    async with websockets.serve(handle_client, "0.0.0.0", BRIDGE_PORT):
        print(f"[Bridge] En attente sur ws://0.0.0.0:{BRIDGE_PORT} ...\n")
        await asyncio.Future()

asyncio.run(main())
