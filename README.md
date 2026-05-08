# QUANTUM TWIN v4 — Guide de démarrage

## Démarrage en 1 commande

```bash
python3 start.py
```

Ce script fait tout automatiquement :
- Compile `src/server.c` → binaire `server`
- Initialise les fichiers `data/`
- Lance un serveur HTTP sur le port 8000
- Lance le bridge WebSocket (ports 8765 + 8766)
- Lance le serveur C sur le port 9090

---

## Liens à ouvrir dans le navigateur

| Page | URL locale | Usage |
|------|-----------|-------|
| **Client** | http://localhost:8000/client.html | Envoyer commandes, messages |
| **Dashboard** | http://localhost:8000/index.html | Voir les logs temps réel |
| **Autre PC / Android** | http://[IP_PC]:8000/client.html | Même réseau Wi-Fi |

---

## Connexion depuis client.html

Dans le panneau ⚙ config :
- **IP** : `127.0.0.1` (local) ou IP du PC (réseau)
- **Port** : `8765`
- **Utilisateur / Mot de passe** : voir ci-dessous

### Comptes disponibles
| Utilisateur | Mot de passe | Rôle |
|-------------|-------------|------|
| admin | secret123 | admin |
| alice | pass123 | user |
| bob | pass456 | user |

---

## Accès depuis internet (Cloudflare)

```bash
# Dans un deuxième terminal, après avoir lancé start.py
cloudflared tunnel --url http://localhost:8765
```

Copie l'URL générée (ex: `https://xxx.trycloudflare.com`) dans le champ IP de client.html, port `443`.

---

## Corrections v4.1 (SYNC / UPDATE / HISTO)

### Problèmes résolus

| Problème | Cause | Fix appliqué |
|----------|-------|-------------|
| SYNC → `ERROR Jumeau inconnu` | Le jumeau devait être connecté au moment du SYNC | Vérification supprimée : la paire est enregistrée même si le jumeau est absent |
| HISTO vide / fichiers introuvables | Chemins `../data/` invalides si lancé depuis la racine | Chemins corrigés en `data/` (relatif à la racine du projet) |
| UPDATE jamais reçu | Conséquence directe du SYNC qui échouait | Résolu par le fix SYNC |

### Comment tester SYNC + UPDATE

1. Lancer `python3 start.py` depuis `quantum_twin_v4/`
2. Ouvrir **deux onglets** sur `http://localhost:8000/client.html`
3. Onglet A : connectez avec `alice / pass123` → vous obtenez `id=1`
4. Onglet B : connectez avec `bob / pass456` → vous obtenez `id=2`
5. Onglet A : tapez `SYNC 2` → réponse `OK SYNC paire(1,2) établie`
6. Onglet A : tapez `STATE ON` → onglet B reçoit automatiquement `UPDATE ON`
7. Vérifiez `data/histo.txt` : toutes les actions y sont maintenant enregistrées

---

## Commandes du protocole

| Commande | Description |
|----------|-------------|
| `CONNECT alice pass123` | S'authentifier |
| `PING` | Tester la connexion (réponse : PONG) |
| `STATE ON` | Changer son état |
| `STATE OFF` | Changer son état |
| `SYNC 2` | S'appairer avec le client ID=2 |
| `MSG 2 Bonjour !` | Envoyer un message au client 2 |
| `LIST` | Lister les clients (admin uniquement) |
| `QUIT` | Se déconnecter |

---

## Architecture

```
[Navigateur PC/Android]
        ↕ WebSocket :8765
[bridge.py]  ←→  [server.c :9090]
        ↕ WebSocket :8766
[Dashboard index.html]
        ↕ HTTP :8000
[start.py → http.server]
```

---

## Prérequis

- Python 3.7+
- gcc + make (Linux/WSL/Mac) : `sudo apt install build-essential`
- `pip install websockets` (fait automatiquement par start.py)
