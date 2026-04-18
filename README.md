# QUANTUM TWIN — Guide d'installation et d'utilisation

## Prérequis

| Appareil   | OS         | Prérequis                          |
|------------|------------|------------------------------------|
| PC 1       | Linux/WSL  | gcc, make, git                     |
| PC 2       | Linux/WSL  | gcc, make, git                     |
| Android    | Android 7+ | Termux (F-Droid)                   |
| iPhone     | iOS 14+    | iSH Shell ou client Python simple  |

---

## Étape 1 — Installer les outils (PC 1 et PC 2)

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install build-essential git -y

# Vérifier
gcc --version
make --version
```

---

## Étape 2 — Récupérer le projet

```bash
git clone https://github.com/votre-groupe/quantum_twin.git
cd quantum_twin
```

Ou décompresser l'archive :

```bash
unzip Groupe_NomA_NomB_NomC_NomD.zip
cd quantum_twin
```

---

## Étape 3 — Compiler

```bash
make all
```

Ou manuellement :

```bash
gcc -Wall -g -o server src/server.c -lpthread
gcc -Wall -g -o client src/client.c -lpthread
```

---

## Étape 4 — Initialiser les fichiers de données

```bash
make init-data
```

Crée dans `data/` :
- `clients.txt` — table des clients
- `pairs.txt`   — paires de jumeaux
- `histo.txt`   — journal d'événements
- `credentials.txt` — identifiants (admin / users)

Comptes par défaut :

| Utilisateur | Mot de passe | Rôle  |
|-------------|--------------|-------|
| admin       | secret123    | admin |
| alice       | pass123      | user  |
| bob         | pass456      | user  |

---

## Étape 5 — Trouver l'IP du serveur (PC 1)

```bash
ip a | grep "inet " | grep -v 127
# ou
hostname -I
```

Exemple : `192.168.1.42`
**Tous les appareils doivent être sur le même réseau Wi-Fi.**

---

## Étape 6 — Lancer le serveur (PC 1)

### Mode TCP (défaut) avec Watchdog :
```bash
./server 9090
```

### Mode UDP (bonus) :
```bash
./server 9090 --udp
```

Le watchdog redémarre automatiquement le serveur en cas de crash.

---

## Étape 7 — Lancer un client (PC 2)

```bash
./client 192.168.1.42 9090
```

### Session exemple :

```
> CONNECT alice pass123
← OK CONNECTED id=1 role=user

> STATE ON
← OK STATE=ON (pas de jumeau)

> SYNC 2
← OK SYNC paire(1,2) établie

> STATE OFF
← OK STATE=OFF twin=2 notifié

> PING
← PONG

> QUIT
← OK Au revoir
```

---

## Étape 8 — Android (Termux)

### Installation Termux :
1. Télécharger Termux depuis **F-Droid** (pas le Play Store)
2. Ouvrir Termux et exécuter :

```bash
pkg update && pkg upgrade -y
pkg install clang git -y
```

### Compiler et lancer le client :
```bash
git clone https://github.com/votre-groupe/quantum_twin.git
cd quantum_twin
gcc -o client src/client.c -lpthread
./client 192.168.1.42 9090
```

---

## Étape 9 — iPhone (iSH Shell)

### Option A — iSH Shell (App Store gratuit) :

```sh
apk add gcc musl-dev
# Copier client.c via AirDrop ou iCloud
gcc -o client client.c
./client 192.168.1.42 9090
```

### Option B — Client Python (plus simple sur iOS) :

Copier le fichier `client_python.py` sur l'iPhone et lancer avec Pythonista ou iSH :

```python
# client_python.py — déjà inclus dans src/
python3 src/client_python.py 192.168.1.42 9090
```

---

## Commandes complètes

| Commande              | Description                            | Rôle requis |
|-----------------------|----------------------------------------|-------------|
| `CONNECT user pass`   | Authentification                       | tous        |
| `STATE ON\|OFF`       | Changer son état + notifier le jumeau  | user        |
| `SYNC [id_jumeau]`    | S'appairer avec un autre client        | user        |
| `PING`                | Vérifier la connexion                  | tous        |
| `LIST`                | Lister tous les clients connectés      | admin       |
| `QUIT`                | Se déconnecter                         | tous        |

---

## Scénario de démonstration (soutenance)

```
Terminal 1 (PC 1) : ./server 9090
Terminal 2 (PC 2) : ./client 192.168.1.42 9090
Terminal 3 (Android) : ./client 192.168.1.42 9090

PC 2 :     CONNECT alice pass123   → OK id=1
Android :  CONNECT bob pass456     → OK id=2
PC 2 :     SYNC 2                  → paire(1,2) créée
PC 2 :     STATE ON                → Android reçoit UPDATE ON
Android :  STATE OFF               → PC 2 reçoit UPDATE OFF
PC 1 :     admin ouvre un 4e terminal, CONNECT admin secret123
Admin :    LIST                    → liste tous les clients
```

---

## Watchdog — comportement

Le serveur démarre en mode watchdog (processus père/fils via `fork()`).

- Si le serveur fils **plante** (signal, segfault…), le père attend 2 secondes et le redémarre.
- Le journal de relances est dans `data/watchdog.log`.
- Pour arrêter proprement : `Ctrl+C` (le serveur fils quitte avec code 0, le watchdog s'arrête).

---

## Structure du projet

```
quantum_twin/
├── src/
│   ├── server.c          — Serveur TCP/UDP parallèle + watchdog
│   ├── client.c          — Client interactif TCP/UDP
│   └── client_python.py  — Client Python pour iPhone
├── data/
│   ├── clients.txt       — État des clients (persistant)
│   ├── pairs.txt         — Paires de jumeaux (persistant)
│   ├── histo.txt         — Journal d'événements
│   ├── credentials.txt   — Identifiants utilisateurs
│   └── watchdog.log      — Journal des redémarrages
├── docs/
│   └── rapport.docx      — Rapport technique
├── Makefile
└── README.md
```
