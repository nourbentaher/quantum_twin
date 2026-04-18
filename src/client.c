/*
 * ============================================================
 *   QUANTUM TWIN — Client
 *   Ecole Nationale d'Ingénieurs de Carthage — PSR 2025-2026
 * ============================================================
 *
 *  Compilation : gcc client.c -o client
 *  Usage TCP   : ./client [ip_serveur] [port]
 *  Usage UDP   : ./client [ip_serveur] [port] --udp
 *
 *  Commandes disponibles (après connexion) :
 *    CONNECT [username] [password]
 *    STATE ON|OFF
 *    SYNC [twin_id]
 *    PING
 *    LIST            (admin seulement)
 *    QUIT
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_IP   "127.0.0.1"
#define DEFAULT_PORT 9090
#define BUF_SIZE     512

/* ─── Thread de réception (écoute le serveur en arrière-plan) ─── */
static void *recv_thread(void *arg)
{
    int sockfd = *(int *)arg;
    char buf[BUF_SIZE];

    while (1) {
        memset(buf, 0, sizeof(buf));
        ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("\n[Serveur déconnecté]\n");
            break;
        }
        /* Affichage immédiat avec indicateur */
        printf("\n← %s", buf);
        printf("> "); fflush(stdout);
    }
    return NULL;
}

/* ─── Mode TCP interactif ─── */
static void run_tcp(const char *ip, int port)
{
    int sockfd;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "IP invalide : %s\n", ip);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        fprintf(stderr,
            "Impossible de joindre le serveur %s:%d\n"
            "Vérifiez que le serveur est lancé et que vous êtes sur le même réseau.\n",
            ip, port);
        exit(1);
    }

    printf("╔══════════════════════════════════════╗\n");
    printf("║   QUANTUM TWIN — Client TCP          ║\n");
    printf("║   Serveur : %-24s║\n", ip);
    printf("║   Port    : %-24d║\n", port);
    printf("╚══════════════════════════════════════╝\n");
    printf("Connecté ! Tapez CONNECT [user] [pass] pour vous authentifier.\n");
    printf("Commandes : CONNECT | STATE | SYNC | PING | LIST | QUIT\n\n");

    /* Thread de réception */
    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, &sockfd);
    pthread_detach(tid);

    /* Boucle de saisie */
    char line[BUF_SIZE];
    while (1) {
        printf("> "); fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;

        /* Envoyer avec \n terminal */
        char msg[BUF_SIZE];
        snprintf(msg, sizeof(msg), "%s\n", line);
        if (send(sockfd, msg, strlen(msg), 0) < 0) {
            perror("send");
            break;
        }
        if (strcmp(line, "QUIT") == 0) break;
    }

    close(sockfd);
    printf("Au revoir.\n");
}

/* ─── Mode UDP simple ─── */
static void run_udp(const char *ip, int port)
{
    int sockfd;
    struct sockaddr_in addr;
    socklen_t alen = sizeof(addr);
    char buf[BUF_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket UDP"); exit(1); }

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "IP invalide : %s\n", ip);
        exit(1);
    }

    printf("╔══════════════════════════════════════╗\n");
    printf("║   QUANTUM TWIN — Client UDP          ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("Commandes : PING | STATE ON|OFF | QUIT\n\n");

    char line[BUF_SIZE];
    while (1) {
        printf("> "); fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) == 0) continue;
        if (strcmp(line, "QUIT") == 0) break;

        sendto(sockfd, line, strlen(line), 0,
               (struct sockaddr *)&addr, alen);

        /* Attendre la réponse (timeout 2s) */
        struct timeval tv = {2, 0};
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        memset(buf, 0, sizeof(buf));
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (n > 0) printf("← %s\n", buf);
        else       printf("[timeout — pas de réponse]\n");
    }
    close(sockfd);
}

/* ─── Point d'entrée ─── */
int main(int argc, char *argv[])
{
    const char *ip   = DEFAULT_IP;
    int         port = DEFAULT_PORT;
    int         udp  = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--udp") == 0) udp = 1;
        else if (atoi(argv[i]) > 0)        port = atoi(argv[i]);
        else                                ip   = argv[i];
    }

    if (udp) run_udp(ip, port);
    else     run_tcp(ip, port);

    return 0;
}
