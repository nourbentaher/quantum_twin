#!/usr/bin/env python3
"""
QUANTUM TWIN — Client Python (équivalent C)
Usage :
    python3 client.py [ip] [port]
    python3 client.py [ip] [port] --udp
"""

import socket
import threading
import sys

DEFAULT_IP = "127.0.0.1"
DEFAULT_PORT = 9090
BUF_SIZE = 512


# ─── Thread réception TCP ───
def recv_loop(sock):
    while True:
        try:
            data = sock.recv(BUF_SIZE)
            if not data:
                print("\n[Serveur déconnecté]")
                break
            print(f"\n← {data.decode()}", end="")
            print("> ", end="", flush=True)
        except:
            break


# ─── Mode TCP ───
def run_tcp(ip, port):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((ip, port))
    except Exception as e:
        print(f"Erreur connexion : {e}")
        print("Vérifiez serveur + réseau.")
        sys.exit(1)

    print("╔══════════════════════════════════════╗")
    print("║   QUANTUM TWIN — Client TCP          ║")
    print(f"║   Serveur : {ip:<24}║")
    print(f"║   Port    : {port:<24}║")
    print("╚══════════════════════════════════════╝")
    print("Connecté ! Tapez CONNECT [user] [pass]")
    print("Commandes : CONNECT | STATE | SYNC | PING | LIST | QUIT\n")

    threading.Thread(target=recv_loop, args=(sock,), daemon=True).start()

    try:
        while True:
            line = input("> ").strip()
            if not line:
                continue

            sock.sendall((line + "\n").encode())

            if line.upper() == "QUIT":
                break
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        sock.close()
        print("Au revoir.")


# ─── Mode UDP ───
def run_udp(ip, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(2)  # timeout 2 secondes

    print("╔══════════════════════════════════════╗")
    print("║   QUANTUM TWIN — Client UDP          ║")
    print("╚══════════════════════════════════════╝")
    print("Commandes : PING | STATE ON|OFF | QUIT\n")

    try:
        while True:
            line = input("> ").strip()
            if not line:
                continue
            if line.upper() == "QUIT":
                break

            sock.sendto(line.encode(), (ip, port))

            try:
                data, _ = sock.recvfrom(BUF_SIZE)
                print(f"← {data.decode()}")
            except socket.timeout:
                print("[timeout — pas de réponse]")

    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        sock.close()


# ─── Main ───
def main():
    ip = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_IP
    port = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_PORT
    udp = len(sys.argv) > 3 and sys.argv[3] == "--udp"

    if udp:
        run_udp(ip, port)
    else:
        run_tcp(ip, port)


if __name__ == "__main__":
    main()
