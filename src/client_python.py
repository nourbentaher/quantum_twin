#!/usr/bin/env python3
"""
QUANTUM TWIN — Client Python (iOS / Pythonista / iSH)
Usage : python3 client_python.py [ip_serveur] [port]
"""

import socket
import threading
import sys

DEFAULT_IP   = "127.0.0.1"
DEFAULT_PORT = 9090

def recv_loop(sock):
    """Thread de réception : affiche les messages du serveur."""
    while True:
        try:
            data = sock.recv(512)
            if not data:
                print("\n[Serveur déconnecté]")
                break
            print(f"\n← {data.decode().strip()}")
            print("> ", end="", flush=True)
        except Exception:
            break

def main():
    ip   = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_IP
    port = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_PORT

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((ip, port))
    except ConnectionRefusedError:
        print(f"Impossible de joindre {ip}:{port}")
        print("Vérifiez que le serveur est lancé et que vous êtes sur le même Wi-Fi.")
        sys.exit(1)

    print("╔══════════════════════════════════════╗")
    print("║   QUANTUM TWIN — Client Python       ║")
    print(f"║   Serveur : {ip:<26}║")
    print(f"║   Port    : {port:<26}║")
    print("╚══════════════════════════════════════╝")
    print("Tapez CONNECT [user] [pass] pour commencer.\n")

    t = threading.Thread(target=recv_loop, args=(sock,), daemon=True)
    t.start()

    try:
        while True:
            print("> ", end="", flush=True)
            line = input()
            if not line:
                continue
            sock.sendall((line + "\n").encode())
            if line.strip().upper() == "QUIT":
                break
    except (EOFError, KeyboardInterrupt):
        pass
    finally:
        sock.close()
        print("Au revoir.")

if __name__ == "__main__":
    main()
