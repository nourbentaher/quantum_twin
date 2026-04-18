/*
 * ============================================================
 *   QUANTUM TWIN — Serveur central
 *   Ecole Nationale d'Ingénieurs de Carthage — PSR 2025-2026
 *   Groupe : Maram HADJ ALI, Douaa BEN MARZOUK,
 *            Nour BEN TAHER, Rached ILEHI
 * ============================================================
 *
 *  Compilation : gcc server.c -o server -lpthread
 *  Usage       : ./server [port]         (TCP, défaut : 9090)
 *                ./server [port] --udp   (UDP — bonus)
 *
 *  Fonctionnalités :
 *    - Serveur TCP parallèle (un thread par client)
 *    - Synchronisation de jumeaux numériques
 *    - Persistance dans clients.txt / pairs.txt / histo.txt
 *    - Mutex sur chaque fichier pour éviter les conflits
 *    - Watchdog : redémarrage automatique si le serveur plante
 *    - Mode UDP (bonus, argument --udp)
 *    - Profils admin / utilisateur (bonus)
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ─── Constantes ─────────────────────────────────── */
#define DEFAULT_PORT    9090
#define MAX_CLIENTS     64
#define BUF_SIZE        512
#define MAX_LINE        256

#define FILE_CLIENTS    "../data/clients.txt"
#define FILE_PAIRS      "../data/pairs.txt"
#define FILE_HISTO      "../data/histo.txt"
#define FILE_CREDS      "../data/credentials.txt"
#define WATCHDOG_LOG    "../data/watchdog.log"

/* ─── Structures ──────────────────────────────────── */
typedef struct {
    int    id;
    char   state[8];       /* "ON" ou "OFF"         */
    char   status[16];     /* "connecte"/"deconnecte"*/
    int    sockfd;         /* socket active (-1 = absent) */
} Client;

typedef struct {
    int client1;
    int client2;
} Pair;

typedef struct {
    int  sockfd;
    char ip[INET_ADDRSTRLEN];
    int  is_admin;         /* 1 = admin, 0 = user   */
    int  client_id;        /* ID attribué après CONNECT */
} ThreadArg;

/* ─── Verrous fichiers ────────────────────────────── */
static pthread_mutex_t mtx_clients = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_pairs   = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx_histo   = PTHREAD_MUTEX_INITIALIZER;

/* ─── Table clients en mémoire ────────────────────── */
static Client  g_clients[MAX_CLIENTS];
static int     g_nb_clients = 0;
static pthread_mutex_t mtx_mem = PTHREAD_MUTEX_INITIALIZER;

/* ─── Indicateur mode UDP ─────────────────────────── */
static int g_use_udp = 0;

/* ═══════════════════════════════════════════════════
   SECTION 1 — Utilitaires de journalisation
   ═══════════════════════════════════════════════════ */

static void log_info(const char *fmt, ...)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", tm);

    va_list ap;
    va_start(ap, fmt);
    printf("[%s] INFO  ", ts);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    fflush(stdout);
}

static void log_warn(const char *fmt, ...)
{
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", tm);

    va_list ap;
    va_start(ap, fmt);
    printf("[%s] WARN  ", ts);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    fflush(stdout);
}

/* ═══════════════════════════════════════════════════
   SECTION 2 — Persistance fichiers
   ═══════════════════════════════════════════════════ */

/* Sauvegarde la table clients en mémoire vers clients.txt */
static void save_clients(void)
{
    pthread_mutex_lock(&mtx_clients);
    FILE *f = fopen(FILE_CLIENTS, "w");
    if (!f) { pthread_mutex_unlock(&mtx_clients); return; }

    fprintf(f, "ID Etat Statut\n");
    pthread_mutex_lock(&mtx_mem);
    for (int i = 0; i < g_nb_clients; i++) {
        fprintf(f, "%d %s %s\n",
                g_clients[i].id,
                g_clients[i].state,
                g_clients[i].status);
    }
    pthread_mutex_unlock(&mtx_mem);
    fclose(f);
    pthread_mutex_unlock(&mtx_clients);
}

/* Charge clients.txt en mémoire au démarrage */
static void load_clients(void)
{
    pthread_mutex_lock(&mtx_clients);
    FILE *f = fopen(FILE_CLIENTS, "r");
    if (!f) {
        /* Fichier inexistant : on le crée vide */
        f = fopen(FILE_CLIENTS, "w");
        if (f) { fprintf(f, "ID Etat Statut\n"); fclose(f); }
        pthread_mutex_unlock(&mtx_clients);
        return;
    }
    char line[MAX_LINE];
    fgets(line, sizeof(line), f); /* ignore entête */

    pthread_mutex_lock(&mtx_mem);
    g_nb_clients = 0;
    while (fgets(line, sizeof(line), f) && g_nb_clients < MAX_CLIENTS) {
        Client *c = &g_clients[g_nb_clients];
        if (sscanf(line, "%d %7s %15s", &c->id, c->state, c->status) == 3) {
            c->sockfd = -1; /* pas encore connecté */
            g_nb_clients++;
        }
    }
    pthread_mutex_unlock(&mtx_mem);
    fclose(f);
    pthread_mutex_unlock(&mtx_clients);
    log_info("Chargement : %d client(s) depuis %s", g_nb_clients, FILE_CLIENTS);
}

/* Enregistre un événement dans histo.txt */
static void append_histo(int client_id, const char *action,
                          const char *valeur, const char *resultat)
{
    pthread_mutex_lock(&mtx_histo);
    FILE *f = fopen(FILE_HISTO, "a");
    if (f) {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        char ts[32];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);
        fprintf(f, "%s | %d | %s | %s | %s\n",
                ts, client_id, action, valeur, resultat);
        fclose(f);
    }
    pthread_mutex_unlock(&mtx_histo);
}

/* Cherche le jumeau d'un client dans pairs.txt, retourne son ID ou -1 */
static int find_twin(int client_id)
{
    pthread_mutex_lock(&mtx_pairs);
    FILE *f = fopen(FILE_PAIRS, "r");
    if (!f) {
        /* Créer pairs.txt vide si absent */
        f = fopen(FILE_PAIRS, "w");
        if (f) { fprintf(f, "Client1 Client2\n"); fclose(f); }
        pthread_mutex_unlock(&mtx_pairs);
        return -1;
    }
    char line[MAX_LINE];
    fgets(line, sizeof(line), f); /* entête */
    int twin = -1;
    while (fgets(line, sizeof(line), f)) {
        int c1, c2;
        if (sscanf(line, "%d %d", &c1, &c2) == 2) {
            if (c1 == client_id) { twin = c2; break; }
            if (c2 == client_id) { twin = c1; break; }
        }
    }
    fclose(f);
    pthread_mutex_unlock(&mtx_pairs);
    return twin;
}

/* Crée une paire (client_id, twin_id) dans pairs.txt si elle n'existe pas */
static int add_pair(int c1, int c2)
{
    if (find_twin(c1) != -1) return 0; /* déjà appariés */

    pthread_mutex_lock(&mtx_pairs);
    FILE *f = fopen(FILE_PAIRS, "a");
    if (!f) { pthread_mutex_unlock(&mtx_pairs); return -1; }
    fprintf(f, "%d %d\n", c1, c2);
    fclose(f);
    pthread_mutex_unlock(&mtx_pairs);
    return 1;
}

/* ═══════════════════════════════════════════════════
   SECTION 3 — Table clients en mémoire
   ═══════════════════════════════════════════════════ */

/* Retourne un pointeur sur le client d'ID donné, ou NULL */
static Client *get_client(int id)
{
    pthread_mutex_lock(&mtx_mem);
    for (int i = 0; i < g_nb_clients; i++) {
        if (g_clients[i].id == id) {
            pthread_mutex_unlock(&mtx_mem);
            return &g_clients[i];
        }
    }
    pthread_mutex_unlock(&mtx_mem);
    return NULL;
}

/* Inscrit un nouveau client, retourne son ID */
static int register_client(int sockfd, const char *ip)
{
    pthread_mutex_lock(&mtx_mem);
    int id = g_nb_clients + 1;
    if (g_nb_clients >= MAX_CLIENTS) {
        pthread_mutex_unlock(&mtx_mem);
        return -1;
    }
    Client *c = &g_clients[g_nb_clients++];
    c->id     = id;
    strcpy(c->state,  "OFF");
    strcpy(c->status, "connecte");
    c->sockfd = sockfd;
    pthread_mutex_unlock(&mtx_mem);

    save_clients();
    log_info("Nouveau client enregistré : ID=%d IP=%s", id, ip);
    return id;
}

/* Met à jour l'état d'un client */
static void update_state(int id, const char *new_state)
{
    Client *c = get_client(id);
    if (!c) return;
    pthread_mutex_lock(&mtx_mem);
    strncpy(c->state, new_state, sizeof(c->state) - 1);
    pthread_mutex_unlock(&mtx_mem);
    save_clients();
}

/* Marque un client comme déconnecté */
static void disconnect_client(int id)
{
    Client *c = get_client(id);
    if (!c) return;
    pthread_mutex_lock(&mtx_mem);
    strcpy(c->status, "deconnecte");
    c->sockfd = -1;
    pthread_mutex_unlock(&mtx_mem);
    save_clients();
}

/* Envoie un message à un client via sa socket active */
static int send_to_client(int target_id, const char *msg)
{
    Client *c = get_client(target_id);
    if (!c || c->sockfd < 0) return -1;
    pthread_mutex_lock(&mtx_mem);
    int fd = c->sockfd;
    pthread_mutex_unlock(&mtx_mem);
    return send(fd, msg, strlen(msg), 0);
}

/* ═══════════════════════════════════════════════════
   SECTION 4 — Authentification (bonus profils)
   ═══════════════════════════════════════════════════ */

/*
 * credentials.txt format :
 *   username password role
 *   admin    secret   admin
 *   alice    pass123  user
 */
static int authenticate(const char *username, const char *password, int *is_admin)
{
    FILE *f = fopen(FILE_CREDS, "r");
    if (!f) {
        /* Pas de fichier = pas d'auth requise, accès user par défaut */
        *is_admin = 0;
        return 1;
    }
    char line[MAX_LINE], u[64], p[64], r[16];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%63s %63s %15s", u, p, r) == 3) {
            if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
                *is_admin = (strcmp(r, "admin") == 0) ? 1 : 0;
                found = 1;
                break;
            }
        }
    }
    fclose(f);
    return found;
}

/* ═══════════════════════════════════════════════════
   SECTION 5 — Protocole de messages
   ═══════════════════════════════════════════════════
 *
 *  Requêtes client → serveur :
 *    CONNECT [username] [password]
 *    STATE [ON|OFF]
 *    SYNC [twin_id]         (demande d'appariement)
 *    PING
 *    LIST                   (admin seulement)
 *    QUIT
 *
 *  Réponses serveur → client :
 *    OK [message]
 *    UPDATE [state]         (propagé au jumeau)
 *    PONG
 *    ERROR [message]
 *    CLIENTS [liste]        (réponse à LIST)
 * ═══════════════════════════════════════════════════ */

static void handle_connect(int sockfd, const char *args,
                            ThreadArg *ta)
{
    char username[64] = "anonymous";
    char password[64] = "";
    sscanf(args, "%63s %63s", username, password);

    int is_admin = 0;
    if (!authenticate(username, password, &is_admin)) {
        send(sockfd, "ERROR Authentification échouée\n", 31, 0);
        append_histo(0, "CONNECT", username, "echec-auth");
        return;
    }

    int id = register_client(sockfd, ta->ip);
    if (id < 0) {
        send(sockfd, "ERROR Serveur plein\n", 20, 0);
        return;
    }
    ta->client_id = id;
    ta->is_admin  = is_admin;

    char rep[BUF_SIZE];
    snprintf(rep, sizeof(rep), "OK CONNECTED id=%d role=%s\n",
             id, is_admin ? "admin" : "user");
    send(sockfd, rep, strlen(rep), 0);
    append_histo(id, "CONNECT", username, "succes");
    log_info("Client %d connecté (%s) — rôle %s",
             id, ta->ip, is_admin ? "admin" : "user");
}

static void handle_state(int sockfd, int client_id, const char *args)
{
    char new_state[8];
    sscanf(args, "%7s", new_state);

    if (strcmp(new_state, "ON") != 0 && strcmp(new_state, "OFF") != 0) {
        send(sockfd, "ERROR État invalide (ON ou OFF)\n", 32, 0);
        return;
    }

    update_state(client_id, new_state);

    /* Propager au jumeau s'il existe */
    int twin_id = find_twin(client_id);
    char rep[BUF_SIZE];

    if (twin_id > 0) {
        char update_msg[BUF_SIZE];
        snprintf(update_msg, sizeof(update_msg),
                 "UPDATE %s from=%d\n", new_state, client_id);
        if (send_to_client(twin_id, update_msg) > 0) {
            append_histo(twin_id, "UPDATE", new_state, "recu");
            update_state(twin_id, new_state);
            snprintf(rep, sizeof(rep),
                     "OK STATE=%s twin=%d notifié\n", new_state, twin_id);
        } else {
            snprintf(rep, sizeof(rep),
                     "OK STATE=%s twin=%d hors-ligne\n", new_state, twin_id);
        }
    } else {
        snprintf(rep, sizeof(rep),
                 "OK STATE=%s (pas de jumeau)\n", new_state);
    }

    send(sockfd, rep, strlen(rep), 0);
    append_histo(client_id, "STATE", new_state, "succes");
    log_info("Client %d → STATE %s", client_id, new_state);
}

static void handle_sync(int sockfd, int client_id, const char *args)
{
    int twin_id = 0;
    sscanf(args, "%d", &twin_id);

    if (twin_id <= 0 || twin_id == client_id) {
        send(sockfd, "ERROR ID jumeau invalide\n", 25, 0);
        return;
    }
    if (!get_client(twin_id)) {
        send(sockfd, "ERROR Jumeau inconnu\n", 21, 0);
        return;
    }

    int res = add_pair(client_id, twin_id);
    char rep[BUF_SIZE];
    if (res >= 0) {
        snprintf(rep, sizeof(rep),
                 "OK SYNC paire(%d,%d) établie\n", client_id, twin_id);
        append_histo(client_id, "SYNC", args, "succes");
        log_info("Paire créée : %d ↔ %d", client_id, twin_id);
    } else {
        snprintf(rep, sizeof(rep), "ERROR SYNC impossible\n");
        append_histo(client_id, "SYNC", args, "echec");
    }
    send(sockfd, rep, strlen(rep), 0);
}

static void handle_list(int sockfd, int is_admin)
{
    if (!is_admin) {
        send(sockfd, "ERROR Accès refusé (admin requis)\n", 34, 0);
        return;
    }
    char rep[BUF_SIZE * 4] = "CLIENTS\n";
    pthread_mutex_lock(&mtx_mem);
    for (int i = 0; i < g_nb_clients; i++) {
        char line[128];
        snprintf(line, sizeof(line), "  id=%d state=%s status=%s\n",
                 g_clients[i].id,
                 g_clients[i].state,
                 g_clients[i].status);
        strncat(rep, line, sizeof(rep) - strlen(rep) - 1);
    }
    pthread_mutex_unlock(&mtx_mem);
    send(sockfd, rep, strlen(rep), 0);
}

/* ═══════════════════════════════════════════════════
   SECTION 6 — Thread de traitement client
   ═══════════════════════════════════════════════════ */

static void *client_thread(void *arg)
{
    ThreadArg *ta = (ThreadArg *)arg;
    int  sockfd    = ta->sockfd;
    char buf[BUF_SIZE];

    log_info("Thread démarré pour %s", ta->ip);

    while (1) {
        memset(buf, 0, sizeof(buf));
        ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break; /* déconnexion ou erreur */

        /* Supprimer \r\n */
        buf[strcspn(buf, "\r\n")] = '\0';
        if (strlen(buf) == 0) continue;

        log_info("Reçu [%s] de client %d", buf, ta->client_id);

        /* Parser commande + arguments */
        char cmd[32] = {0};
        char args[BUF_SIZE] = {0};
        sscanf(buf, "%31s %479[^\n]", cmd, args);

        if (strcmp(cmd, "CONNECT") == 0) {
            handle_connect(sockfd, args, ta);

        } else if (ta->client_id <= 0) {
            /* Toute autre commande nécessite d'être connecté */
            send(sockfd, "ERROR Non connecté — utilisez CONNECT\n", 38, 0);

        } else if (strcmp(cmd, "STATE") == 0) {
            handle_state(sockfd, ta->client_id, args);

        } else if (strcmp(cmd, "SYNC") == 0) {
            handle_sync(sockfd, ta->client_id, args);

        } else if (strcmp(cmd, "PING") == 0) {
            send(sockfd, "PONG\n", 5, 0);

        } else if (strcmp(cmd, "LIST") == 0) {
            handle_list(sockfd, ta->is_admin);

        } else if (strcmp(cmd, "QUIT") == 0) {
            send(sockfd, "OK Au revoir\n", 13, 0);
            break;

        } else {
            send(sockfd, "ERROR Commande inconnue\n", 24, 0);
        }
    }

    if (ta->client_id > 0) {
        disconnect_client(ta->client_id);
        append_histo(ta->client_id, "DISCONNECT", "-", "ok");
        log_info("Client %d déconnecté", ta->client_id);
    }
    close(sockfd);
    free(ta);
    return NULL;
}

/* ═══════════════════════════════════════════════════
   SECTION 7 — Boucle principale TCP
   ═══════════════════════════════════════════════════ */

static void run_tcp(int port)
{
    int server_fd;
    struct sockaddr_in addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    /* Réutilisation du port après redémarrage */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen"); exit(1);
    }

    log_info("Serveur TCP démarré sur le port %d", port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            continue;
        }

        ThreadArg *ta = calloc(1, sizeof(ThreadArg));
        ta->sockfd    = client_fd;
        ta->client_id = 0;
        ta->is_admin  = 0;
        inet_ntop(AF_INET, &client_addr.sin_addr, ta->ip, INET_ADDRSTRLEN);

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, ta) != 0) {
            log_warn("Impossible de créer le thread pour %s", ta->ip);
            close(client_fd);
            free(ta);
        } else {
            pthread_detach(tid); /* libération automatique à la fin */
        }
    }
    close(server_fd);
}

/* ═══════════════════════════════════════════════════
   SECTION 8 — Mode UDP (bonus)
   ═══════════════════════════════════════════════════ */

static void run_udp(int port)
{
    int sockfd;
    struct sockaddr_in addr, client_addr;
    socklen_t clen = sizeof(client_addr);
    char buf[BUF_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket UDP"); exit(1); }

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind UDP"); exit(1);
    }

    log_info("Serveur UDP démarré sur le port %d", port);

    while (1) {
        memset(buf, 0, sizeof(buf));
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&client_addr, &clen);
        if (n <= 0) continue;
        buf[strcspn(buf, "\r\n")] = '\0';

        char rep[BUF_SIZE];
        if (strcmp(buf, "PING") == 0) {
            snprintf(rep, sizeof(rep), "PONG\n");
        } else if (strncmp(buf, "STATE ", 6) == 0) {
            snprintf(rep, sizeof(rep), "OK UDP STATE reçu : %s\n", buf + 6);
            append_histo(0, "UDP-STATE", buf + 6, "recu");
        } else {
            snprintf(rep, sizeof(rep), "ERROR commande UDP inconnue\n");
        }
        sendto(sockfd, rep, strlen(rep), 0,
               (struct sockaddr *)&client_addr, clen);
    }
    close(sockfd);
}

/* ═══════════════════════════════════════════════════
   SECTION 9 — Watchdog (bonus sécurité)
   ═══════════════════════════════════════════════════
 *
 *  Le watchdog fork() un processus fils (le serveur réel).
 *  Si ce fils plante (signal ou code d'erreur), le père
 *  attend 2 secondes puis le redémarre automatiquement.
 *  Toutes les relances sont journalisées dans watchdog.log.
 * ═══════════════════════════════════════════════════ */

static void watchdog_log(const char *msg)
{
    FILE *f = fopen(WATCHDOG_LOG, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(f, "[%s] %s\n", ts, msg);
    fclose(f);
}

static void run_with_watchdog(int port)
{
    int restarts = 0;
    printf("╔══════════════════════════════════╗\n");
    printf("║   QUANTUM TWIN — Watchdog actif  ║\n");
    printf("╚══════════════════════════════════╝\n");
    watchdog_log("Watchdog démarré");

    while (1) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork watchdog");
            exit(1);
        }

        if (pid == 0) {
            /* ─── Processus fils : serveur réel ─── */
            if (g_use_udp)
                run_udp(port);
            else
                run_tcp(port);
            exit(0);
        }

        /* ─── Processus père : surveillance ─── */
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            /* Arrêt propre */
            watchdog_log("Serveur arrêté proprement");
            printf("Serveur arrêté proprement. Watchdog terminé.\n");
            break;
        }

        restarts++;
        char msg[128];
        if (WIFSIGNALED(status)) {
            snprintf(msg, sizeof(msg),
                     "CRASH signal=%d — redémarrage #%d dans 2s",
                     WTERMSIG(status), restarts);
        } else {
            snprintf(msg, sizeof(msg),
                     "SORTIE code=%d — redémarrage #%d dans 2s",
                     WEXITSTATUS(status), restarts);
        }
        watchdog_log(msg);
        fprintf(stderr, "\n⚠  Watchdog : %s\n\n", msg);
        sleep(2);
    }
}

/* ═══════════════════════════════════════════════════
   SECTION 10 — Point d'entrée
   ═══════════════════════════════════════════════════ */

int main(int argc, char *argv[])
{
    int port = DEFAULT_PORT;

    /* Ignorer SIGPIPE pour éviter un crash sur socket fermée */
    signal(SIGPIPE, SIG_IGN);

    /* ─── Analyse des arguments ─── */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--udp") == 0) {
            g_use_udp = 1;
        } else {
            int p = atoi(argv[i]);
            if (p > 0) port = p;
        }
    }

    printf("╔══════════════════════════════════════╗\n");
    printf("║   QUANTUM TWIN — Serveur central     ║\n");
    printf("║   Mode : %-27s║\n", g_use_udp ? "UDP" : "TCP (parallèle)");
    printf("║   Port : %-27d║\n", port);
    printf("╚══════════════════════════════════════╝\n");

    /* Initialiser les fichiers de données */
    load_clients();

    /* Initialiser histo.txt s'il n'existe pas */
    {
        FILE *f = fopen(FILE_HISTO, "a");
        if (f) fclose(f);
    }

    /* Lancer avec watchdog */
    run_with_watchdog(port);

    return 0;
}
