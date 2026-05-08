#!/usr/bin/env python3
"""
QUANTUM TWIN v4 — Lanceur tout-en-un
======================================
  1. Installe websockets si absent
  2. Compile server.c si nécessaire
  3. Init les fichiers data/
  4. Lance le serveur HTTP :8000 (client.html + index.html)
  5. Lance bridge.py (serveur C + WebSocket :8765 + logs :8766)

Usage : python3 start.py
"""

import subprocess, sys, os, socket, threading, time
import http.server, socketserver

HTTP_PORT         = 8000
SERVER_C_PORT     = 9090
CLIENT_WS_PORT    = 8765
DASHBOARD_WS_PORT = 8766


# ─── Couleurs terminal ───
def green(s):  return f"\033[92m{s}\033[0m"
def cyan(s):   return f"\033[96m{s}\033[0m"
def amber(s):  return f"\033[93m{s}\033[0m"
def red(s):    return f"\033[91m{s}\033[0m"
def bold(s):   return f"\033[1m{s}\033[0m"


def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"


def install_websockets():
    try:
        import websockets
        return True
    except ImportError:
        print(amber("[Start] Installation de websockets..."))
        r = subprocess.run(
            [sys.executable, "-m", "pip", "install", "websockets",
             "--break-system-packages", "-q"],
            capture_output=True
        )
        if r.returncode == 0:
            print(green("[Start] ✓ websockets installé"))
            return True
        # Essai sans --break-system-packages (Windows/venv)
        r2 = subprocess.run(
            [sys.executable, "-m", "pip", "install", "websockets", "-q"],
            capture_output=True
        )
        return r2.returncode == 0


def compile_server():
    if not os.path.isfile("src/server.c"):
        print(red("[Start] ✗ src/server.c introuvable"))
        return False

    # Recompiler si server.c est plus récent que le binaire existant
    need_compile = True
    if os.path.isfile("./server") and os.access("./server", os.X_OK):
        src_mtime = os.path.getmtime("src/server.c")
        bin_mtime = os.path.getmtime("./server")
        if bin_mtime >= src_mtime:
            print(green("[Start] ✓ Binaire 'server' à jour"))
            need_compile = False

    if not need_compile:
        return True

    print(amber("[Start] Compilation de server.c..."))
    r = subprocess.run(
        ["gcc", "-Wall", "-g", "-o", "server", "src/server.c", "-lpthread"],
        capture_output=True, text=True
    )
    if r.returncode == 0:
        print(green("[Start] ✓ Compilation réussie"))
        return True
    print(red(f"[Start] ✗ Erreur compilation :\n{r.stderr}"))
    print(amber("[Start] → Installe gcc : sudo apt install build-essential"))
    return False


def init_data():
    os.makedirs("data", exist_ok=True)
    defaults = {
        "data/credentials.txt": "admin secret123 admin\nalice pass123 user\nbob pass456 user\n",
        "data/clients.txt":     "ID Etat Statut\n",
        "data/pairs.txt":       "Client1 Client2\n",
        "data/histo.txt":       "",
        "data/watchdog.log":    "",
    }
    for path, content in defaults.items():
        if not os.path.exists(path):
            with open(path, "w", encoding="utf-8") as f:
                f.write(content)
    print(green("[Start] ✓ Fichiers data/ initialisés"))


def start_http_server():
    """Serveur HTTP silencieux pour servir client.html et index.html."""
    class SilentHandler(http.server.SimpleHTTPRequestHandler):
        def log_message(self, *args):
            pass
    def run():
        # Permettre réutilisation du port
        socketserver.TCPServer.allow_reuse_address = True
        with socketserver.TCPServer(("", HTTP_PORT), SilentHandler) as httpd:
            httpd.serve_forever()
    t = threading.Thread(target=run, daemon=True)
    t.start()
    print(green(f"[Start] ✓ Serveur HTTP démarré sur port {HTTP_PORT}"))


def print_urls(local_ip):
    print()
    print(bold("━" * 60))
    print(bold(cyan("  OUVRIR CES LIENS DANS LE NAVIGATEUR :")))
    print()
    print(bold("  📱  CLIENT (envoyer commandes / messages) :"))
    print(f"       {green('http://localhost:' + str(HTTP_PORT) + '/client.html')}")
    print(f"       {cyan('http://' + local_ip + ':' + str(HTTP_PORT) + '/client.html')}  ← Android / autre PC")
    print()
    print(bold("  🖥️   DASHBOARD SERVEUR (logs temps réel) :"))
    print(f"       {green('http://localhost:' + str(HTTP_PORT) + '/index.html')}")
    print(f"       {cyan('http://' + local_ip + ':' + str(HTTP_PORT) + '/index.html')}   ← autre PC")
    print()
    print(bold("  ⚙️   Dans client.html — paramètres de connexion :"))
    print(f"       IP   : {cyan(local_ip)}     Port : {cyan(str(CLIENT_WS_PORT))}")
    print()
    print(bold("  ☁️   Via Cloudflare (accès depuis n'importe où) :"))
    print(f"       {amber('cloudflared tunnel --url http://localhost:' + str(CLIENT_WS_PORT))}")
    print(f"       Puis dans client.html : colle l'URL .trycloudflare.com / port 443")
    print(bold("━" * 60))
    print()
    print(amber("  Comptes disponibles :"))
    print("    admin / secret123    (rôle admin)")
    print("    alice / pass123      (rôle user)")
    print("    bob   / pass456      (rôle user)")
    print(bold("━" * 60))
    print()


def main():
    print()
    print(bold(cyan("╔═══════════════════════════════════════════════╗")))
    print(bold(cyan("║   QUANTUM TWIN v4 — Démarrage automatique     ║")))
    print(bold(cyan("╚═══════════════════════════════════════════════╝")))
    print()

    # 1. websockets
    if not install_websockets():
        print(red("[Start] websockets introuvable — pip install websockets"))
        sys.exit(1)

    # 2. Compiler
    ok = compile_server()
    # Ne pas bloquer si gcc absent : bridge.py affichera un warning

    # 3. Init données
    init_data()

    # 4. Serveur HTTP
    start_http_server()

    # 5. Afficher URLs
    local_ip = get_local_ip()
    print_urls(local_ip)

    # 6. Lancer bridge (bloquant — capture Ctrl+C)
    print(bold("[Start] Lancement du bridge... (Ctrl+C pour arrêter)\n"))
    try:
        subprocess.run([sys.executable, "bridge.py", "127.0.0.1", str(SERVER_C_PORT)])
    except KeyboardInterrupt:
        print(f"\n{amber('[Start] Arrêt propre.')}")


if __name__ == "__main__":
    main()
