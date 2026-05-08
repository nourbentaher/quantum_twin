#!/usr/bin/env python3
"""
QUANTUM TWIN v4 — Bridge unifié
================================
Ce fichier fait TOUT :
  - Lance le serveur C (src/server.c compilé → ./server)
  - Capture stdout du serveur C ligne par ligne
  - Diffuse ces logs en temps réel à tous les dashboards (port 8766)
  - Relaie les clients web ↔ serveur C via TCP (port 8765)

Démarrage : python3 bridge.py
             python3 bridge.py 127.0.0.1 9090
"""

import asyncio, socket, subprocess, sys, os, threading, time, json, datetime

# ── Installer websockets si absent ──
try:
    import websockets
except ImportError:
    print("[Bridge] Installation de websockets...")
    subprocess.run([sys.executable, "-m", "pip", "install", "websockets",
                    "--break-system-packages"], capture_output=True)
    import websockets

SERVER_IP         = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
SERVER_PORT       = int(sys.argv[2]) if len(sys.argv) > 2 else 9090
CLIENT_WS_PORT    = 8765
DASHBOARD_WS_PORT = 8766

_loop            = None
_log_queue       = None
_dashboard_set   = set()
_server_proc     = None


# ════════════════════════════════════════════════════
#  UTILITAIRES
# ════════════════════════════════════════════════════

def ts():
    return datetime.datetime.now().strftime("%H:%M:%S")

def classify(line):
    """Détermine le niveau d'un log à partir de son texte."""
    l = line.lower()
    if "warn"  in l or "hors-ligne" in l:                  return "warn"
    if "error" in l or "impossible" in l or "plante" in l: return "error"
    if "nouveau client" in l or "connecté" in l:           return "success"
    if "état" in l or "state" in l or "update" in l:       return "state"
    if "paire" in l or "sync" in l:                        return "sync"
    if "msg"   in l:                                        return "msg"
    if "déconnecté" in l or "disconnect" in l:             return "disconnect"
    if "watchdog" in l or "redémarr" in l or "crash" in l: return "watchdog"
    if "warn"  in line.upper():                             return "warn"
    return "info"

async def broadcast_log(text):
    """Envoie un log JSON à tous les dashboards connectés."""
    if not _dashboard_set:
        return
    payload = json.dumps({"type": "log", "level": classify(text), "text": text})
    dead = set()
    for ws in list(_dashboard_set):
        try:
            await ws.send(payload)
        except Exception:
            dead.add(ws)
    _dashboard_set.difference_update(dead)


# ════════════════════════════════════════════════════
#  LANCEMENT SERVEUR C + CAPTURE STDOUT
# ════════════════════════════════════════════════════

def find_server_binary():
    for p in ["./server", "./build/server", "server"]:
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    return None

def server_reader_thread():
    """Thread qui lit stdout du serveur C et pousse dans la queue."""
    global _server_proc
    binary = find_server_binary()
    if not binary:
        msg = f"[{ts()}] WARN  Binaire 'server' introuvable — lance 'make' d'abord"
        if _loop and _log_queue:
            asyncio.run_coroutine_threadsafe(_log_queue.put(msg), _loop)
        return

    print(f"[Bridge] Lancement : {binary} {SERVER_PORT}")
    while True:
        try:
            _server_proc = subprocess.Popen(
                [binary, str(SERVER_PORT)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True, bufsize=1
            )
            for raw in _server_proc.stdout:
                line = raw.rstrip("\n")
                if line:
                    print(line)   # aussi visible dans le terminal
                    if _loop and _log_queue:
                        asyncio.run_coroutine_threadsafe(_log_queue.put(line), _loop)
            _server_proc.wait()
            msg = f"[{ts()}] WARN  Serveur C terminé (code {_server_proc.returncode}) — redémarrage 2s..."
            if _loop and _log_queue:
                asyncio.run_coroutine_threadsafe(_log_queue.put(msg), _loop)
            time.sleep(2)
        except Exception as e:
            msg = f"[{ts()}] ERROR Erreur serveur C : {e}"
            if _loop and _log_queue:
                asyncio.run_coroutine_threadsafe(_log_queue.put(msg), _loop)
            time.sleep(3)


# ════════════════════════════════════════════════════
#  LOG BROADCASTER (coroutine asyncio)
# ════════════════════════════════════════════════════

async def log_broadcaster():
    """Consomme la queue et diffuse chaque log aux dashboards."""
    while True:
        try:
            line = await asyncio.wait_for(_log_queue.get(), timeout=0.5)
            await broadcast_log(line)
        except asyncio.TimeoutError:
            pass
        except Exception:
            await asyncio.sleep(0.05)


# ════════════════════════════════════════════════════
#  HANDLER CLIENTS WEB → TCP → SERVEUR C  (port 8765)
# ════════════════════════════════════════════════════

async def handle_client(websocket):
    peer = websocket.remote_address
    print(f"[Bridge] Client web connecté : {peer[0]}:{peer[1]}")
    await _log_queue.put(f"[{ts()}] INFO  Nouveau client web depuis {peer[0]}")

    # Attendre que le serveur C soit prêt (max 5s)
    connected = False
    for _ in range(10):
        try:
            tcp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp.connect((SERVER_IP, SERVER_PORT))
            tcp.setblocking(False)
            connected = True
            break
        except Exception:
            tcp.close()
            await asyncio.sleep(0.5)

    if not connected:
        err = f"ERROR Serveur C injoignable sur {SERVER_IP}:{SERVER_PORT}\n"
        await websocket.send(err)
        return

    loop = asyncio.get_event_loop()

    async def tcp_to_ws():
        while True:
            try:
                data = await loop.sock_recv(tcp, 1024)
                if not data:
                    break
                await websocket.send(data.decode("utf-8", errors="replace"))
            except Exception:
                break

    async def ws_to_tcp():
        try:
            async for msg in websocket:
                line = msg.strip() + "\n"
                await loop.sock_sendall(tcp, line.encode())
        except Exception:
            pass

    tasks = [asyncio.create_task(tcp_to_ws()),
             asyncio.create_task(ws_to_tcp())]
    done, pending = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
    for t in pending:
        t.cancel()
    try:
        tcp.close()
    except Exception:
        pass

    print(f"[Bridge] Client web déconnecté : {peer[0]}")
    await _log_queue.put(f"[{ts()}] INFO  Client web {peer[0]} déconnecté du bridge")


# ════════════════════════════════════════════════════
#  HANDLER DASHBOARD  (port 8766)
# ════════════════════════════════════════════════════

async def handle_dashboard(websocket):
    _dashboard_set.add(websocket)
    print(f"[Bridge] Dashboard connecté ({len(_dashboard_set)} total)")
    try:
        await websocket.send(json.dumps({
            "type": "log", "level": "success",
            "text": f"[{ts()}] INFO  Dashboard connecté — logs temps réel actifs"
        }))
        await websocket.wait_closed()
    finally:
        _dashboard_set.discard(websocket)


# ════════════════════════════════════════════════════
#  MAIN
# ════════════════════════════════════════════════════

async def main():
    global _loop, _log_queue
    _loop      = asyncio.get_event_loop()
    _log_queue = asyncio.Queue()

    print("╔═══════════════════════════════════════════╗")
    print("║   QUANTUM TWIN v4 — Bridge unifié         ║")
    print(f"║   Serveur C     : {SERVER_IP}:{SERVER_PORT:<20}║")
    print(f"║   Clients WS    : ws://0.0.0.0:{CLIENT_WS_PORT}           ║")
    print(f"║   Dashboard WS  : ws://0.0.0.0:{DASHBOARD_WS_PORT}           ║")
    print("╚═══════════════════════════════════════════╝")
    print()

    # Lancer le thread de capture serveur C
    t = threading.Thread(target=server_reader_thread, daemon=True)
    t.start()

    # Laisser le serveur C démarrer
    await asyncio.sleep(1.5)

    # Démarrer les deux serveurs WebSocket
    async with websockets.serve(handle_client,    "0.0.0.0", CLIENT_WS_PORT):
        async with websockets.serve(handle_dashboard, "0.0.0.0", DASHBOARD_WS_PORT):
            print(f"[Bridge] ✓ Clients   → ws://0.0.0.0:{CLIENT_WS_PORT}")
            print(f"[Bridge] ✓ Dashboard → ws://0.0.0.0:{DASHBOARD_WS_PORT}")
            print()
            await log_broadcaster()

asyncio.run(main())
