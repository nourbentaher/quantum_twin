# 🖥️ QUANTUM TWIN — Guide d'exécution

## PC Serveur (Rached)

**1 — Ouvrir Ubuntu WSL**
```bash
wsl
```

**2 — Lancer le serveur**
```bash
cd ~/quantum_twin
./server 9090
```

**3 — Lancer le dashboard (nouveau terminal)**
```bash
cd ~/quantum_twin
python3 -m http.server 8080
```

**4 — Ouvrir le dashboard dans Chrome**
http://172.22.141.234:8080/web/index.html

---

## PC Client (camarade)

**1 — Compiler le client**
```bash
gcc -o client src/client.c -lpthread
```

**2 — Se connecter au serveur**
```bash
./client 192.168.0.120 9090
```

**3 — Commandes disponibles**
CONNECT alice pass123    → connexion
STATE ON                 → activer état
STATE OFF                → désactiver état
SYNC 2                   → jumeau avec client ID=2
PING                     → tester connexion
LIST                     → voir tous les clients (admin)
QUIT                     → déconnexion

---

## Scénario de démonstration
Client 1 : CONNECT alice pass123
Client 2 : CONNECT bob pass456
Client 1 : SYNC 2
Client 1 : STATE ON
→ bob reçoit automatiquement : UPDATE ON

---

## Identifiants

| Utilisateur | Mot de passe | Rôle  |
|-------------|-------------|-------|
| admin       | secret123   | admin |
| alice       | pass123     | user  |
| bob         | pass456     | user  |

---

**IP serveur Wi-Fi : `192.168.0.120` | Port : `9090`**
