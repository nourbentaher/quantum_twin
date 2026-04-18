const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  Header, Footer, AlignmentType, HeadingLevel, LevelFormat,
  BorderStyle, WidthType, ShadingType, VerticalAlign,
  PageNumber, PageBreak, TabStopType, TabStopPosition
} = require('docx');
const fs = require('fs');

const BLUE  = "1F4E79";
const LBLUE = "D6E4F0";
const GRAY  = "F2F2F2";
const WHITE = "FFFFFF";

const border = { style: BorderStyle.SINGLE, size: 1, color: "AAAAAA" };
const borders = { top: border, bottom: border, left: border, right: border };
const noBorder = { style: BorderStyle.NONE, size: 0, color: "FFFFFF" };
const noBorders = { top: noBorder, bottom: noBorder, left: noBorder, right: noBorder };

function heading1(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_1,
    children: [new TextRun({ text, bold: true, size: 32, color: BLUE, font: "Arial" })],
    spacing: { before: 360, after: 200 },
    border: { bottom: { style: BorderStyle.SINGLE, size: 6, color: BLUE, space: 4 } }
  });
}

function heading2(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_2,
    children: [new TextRun({ text, bold: true, size: 26, color: "2E75B6", font: "Arial" })],
    spacing: { before: 280, after: 140 }
  });
}

function heading3(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_3,
    children: [new TextRun({ text, bold: true, size: 22, color: "2E75B6", font: "Arial" })],
    spacing: { before: 200, after: 100 }
  });
}

function para(text, opts = {}) {
  return new Paragraph({
    children: [new TextRun({ text, size: 22, font: "Arial", ...opts })],
    spacing: { after: 160 },
    alignment: AlignmentType.JUSTIFIED
  });
}

function bullet(text) {
  return new Paragraph({
    numbering: { reference: "bullets", level: 0 },
    children: [new TextRun({ text, size: 22, font: "Arial" })],
    spacing: { after: 80 }
  });
}

function numbered(text) {
  return new Paragraph({
    numbering: { reference: "numbers", level: 0 },
    children: [new TextRun({ text, size: 22, font: "Arial" })],
    spacing: { after: 80 }
  });
}

function codeBlock(text) {
  return new Paragraph({
    children: [new TextRun({ text, size: 18, font: "Courier New", color: "2D2D2D" })],
    spacing: { before: 80, after: 80 },
    shading: { fill: "F5F5F5", type: ShadingType.CLEAR },
    indent: { left: 360 }
  });
}

function makeTable(headers, rows, colWidths) {
  const totalW = colWidths.reduce((a, b) => a + b, 0);
  return new Table({
    width: { size: totalW, type: WidthType.DXA },
    columnWidths: colWidths,
    rows: [
      new TableRow({
        tableHeader: true,
        children: headers.map((h, i) => new TableCell({
          borders,
          width: { size: colWidths[i], type: WidthType.DXA },
          shading: { fill: BLUE, type: ShadingType.CLEAR },
          margins: { top: 80, bottom: 80, left: 120, right: 120 },
          children: [new Paragraph({
            children: [new TextRun({ text: h, bold: true, size: 20, color: WHITE, font: "Arial" })],
            alignment: AlignmentType.CENTER
          })]
        }))
      }),
      ...rows.map((row, ri) => new TableRow({
        children: row.map((cell, i) => new TableCell({
          borders,
          width: { size: colWidths[i], type: WidthType.DXA },
          shading: { fill: ri % 2 === 0 ? WHITE : GRAY, type: ShadingType.CLEAR },
          margins: { top: 80, bottom: 80, left: 120, right: 120 },
          children: [new Paragraph({
            children: [new TextRun({ text: cell, size: 20, font: "Arial" })],
            alignment: i === 0 ? AlignmentType.LEFT : AlignmentType.CENTER
          })]
        }))
      }))
    ]
  });
}

function spacer() {
  return new Paragraph({ children: [new TextRun("")], spacing: { after: 120 } });
}

function pageBreak() {
  return new Paragraph({ children: [new PageBreak()] });
}

const doc = new Document({
  numbering: {
    config: [
      { reference: "bullets", levels: [{ level: 0, format: LevelFormat.BULLET, text: "•",
          alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 720, hanging: 360 } } } }] },
      { reference: "numbers", levels: [{ level: 0, format: LevelFormat.DECIMAL, text: "%1.",
          alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 720, hanging: 360 } } } }] },
    ]
  },
  styles: {
    default: { document: { run: { font: "Arial", size: 22 } } },
    paragraphStyles: [
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 32, bold: true, font: "Arial", color: BLUE },
        paragraph: { spacing: { before: 360, after: 200 }, outlineLevel: 0 } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 26, bold: true, font: "Arial", color: "2E75B6" },
        paragraph: { spacing: { before: 280, after: 140 }, outlineLevel: 1 } },
      { id: "Heading3", name: "Heading 3", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 22, bold: true, font: "Arial", color: "2E75B6" },
        paragraph: { spacing: { before: 200, after: 100 }, outlineLevel: 2 } },
    ]
  },
  sections: [{
    properties: {
      page: {
        size: { width: 11906, height: 16838 },
        margin: { top: 1440, right: 1134, bottom: 1440, left: 1134 }
      }
    },
    headers: {
      default: new Header({
        children: [
          new Paragraph({
            children: [
              new TextRun({ text: "QUANTUM TWIN", bold: true, size: 18, color: BLUE, font: "Arial" }),
              new TextRun({ text: "   |   Programmation des Systèmes Réseaux — ENICarthage 2025–2026", size: 18, color: "888888", font: "Arial" })
            ],
            border: { bottom: { style: BorderStyle.SINGLE, size: 4, color: BLUE, space: 4 } }
          })
        ]
      })
    },
    footers: {
      default: new Footer({
        children: [
          new Paragraph({
            children: [
              new TextRun({ text: "Maram HADJ ALI  •  Douaa BEN MARZOUK  •  Nour BEN TAHER  •  Rached ILEHI", size: 16, color: "888888", font: "Arial" }),
            ],
            border: { top: { style: BorderStyle.SINGLE, size: 4, color: BLUE, space: 4 } },
            tabStops: [{ type: TabStopType.RIGHT, position: TabStopPosition.MAX }],
          })
        ]
      })
    },
    children: [

      /* ══════════════ PAGE DE GARDE ══════════════ */
      new Paragraph({
        children: [new TextRun("")],
        spacing: { before: 1440, after: 0 }
      }),
      new Paragraph({
        children: [new TextRun({ text: "Ecole Nationale d'Ingénieurs de Carthage", bold: true, size: 28, font: "Arial", color: BLUE })],
        alignment: AlignmentType.CENTER, spacing: { after: 120 }
      }),
      new Paragraph({
        children: [new TextRun({ text: "Université de Carthage  —  2ème année Génie Informatique", size: 22, font: "Arial", color: "444444" })],
        alignment: AlignmentType.CENTER, spacing: { after: 600 }
      }),
      new Paragraph({
        children: [new TextRun({ text: "SYSTÈME DE SYNCHRONISATION", bold: true, size: 52, font: "Arial", color: BLUE })],
        alignment: AlignmentType.CENTER, spacing: { after: 160 },
        border: { top: { style: BorderStyle.SINGLE, size: 12, color: BLUE, space: 8 } }
      }),
      new Paragraph({
        children: [new TextRun({ text: "QUANTUM TWIN", bold: true, size: 64, font: "Arial", color: "2E75B6" })],
        alignment: AlignmentType.CENTER, spacing: { after: 160 },
        border: { bottom: { style: BorderStyle.SINGLE, size: 12, color: BLUE, space: 8 } }
      }),
      new Paragraph({ children: [new TextRun("")], spacing: { after: 400 } }),
      new Paragraph({
        children: [new TextRun({ text: "Programmation des Systèmes Réseaux", size: 24, font: "Arial", color: "555555", italics: true })],
        alignment: AlignmentType.CENTER, spacing: { after: 80 }
      }),
      new Paragraph({
        children: [new TextRun({ text: "Encadré par Mme Khaoula BEDOUI", size: 22, font: "Arial", color: "555555" })],
        alignment: AlignmentType.CENTER, spacing: { after: 600 }
      }),
      new Paragraph({
        children: [new TextRun({ text: "Réalisé par :", bold: true, size: 24, font: "Arial", color: BLUE })],
        alignment: AlignmentType.CENTER, spacing: { after: 120 }
      }),
      new Paragraph({
        children: [new TextRun({ text: "Maram HADJ ALI       Douaa BEN MARZOUK       Nour BEN TAHER       Rached ILEHI", size: 22, font: "Arial", color: "333333" })],
        alignment: AlignmentType.CENTER, spacing: { after: 600 }
      }),
      new Paragraph({
        children: [new TextRun({ text: "Année universitaire 2025 – 2026", size: 22, font: "Arial", color: "777777" })],
        alignment: AlignmentType.CENTER
      }),

      pageBreak(),

      /* ══════════════ RÉSUMÉ ══════════════ */
      heading1("Résumé"),
      para("Ce projet consiste à concevoir un système informatique de synchronisation entre appareils distants, inspiré du concept d'intrication quantique. Le système permet de créer des couples de clients appelés jumeaux numériques, partageant un état commun synchronisé en temps réel via un serveur central."),
      para("Toute modification effectuée par un client est immédiatement répercutée sur son jumeau. Le projet met en œuvre des concepts avancés en réseaux (TCP/UDP), en programmation parallèle (multi-threading avec pthreads) et en gestion de données persistantes via des fichiers texte."),
      para("Un mécanisme de watchdog a été ajouté en bonus pour garantir la disponibilité continue du serveur : en cas de crash, le serveur redémarre automatiquement. Le contrôle d'accès par profils (admin / utilisateur) constitue le second bonus implémenté."),
      spacer(),

      /* ══════════════ 1. ORGANISATION DU TRAVAIL ══════════════ */
      heading1("1. Organisation du travail"),
      heading2("1.1 Répartition des tâches"),
      para("Le projet a été réalisé en quadrinôme. La répartition suivante a été adoptée, avec des révisions mutuelles entre membres :"),
      spacer(),
      makeTable(
        ["Membre", "Tâches principales"],
        [
          ["Maram HADJ ALI", "Serveur TCP — socket, bind, listen, accept"],
          ["Douaa BEN MARZOUK", "Serveur parallèle — pthreads, mutex, synchronisation"],
          ["Nour BEN TAHER", "Persistance fichiers — lecture/écriture, rechargement"],
          ["Rached ILEHI", "Client C/Python, tests multi-appareils, Makefile, README"],
        ],
        [3000, 6000]
      ),
      spacer(),
      heading2("1.2 Calendrier"),
      numbered("Jour 1 : Définition des fichiers de données et du protocole de messages"),
      numbered("Jour 2 : Communication TCP de base (serveur séquentiel + client minimal)"),
      numbered("Jours 3–4 : Opérations métier (STATE, SYNC, PING, historique)"),
      numbered("Jour 5 : Serveur parallèle — intégration pthread + mutex"),
      numbered("Jour 6 : Tests sur 4 appareils (PC, Android, iPhone)"),
      numbered("Jours 7–8 : Bonus (watchdog, profils), nettoyage du code, rapport, préparation soutenance"),
      spacer(),

      /* ══════════════ 2. ARCHITECTURE ══════════════ */
      pageBreak(),
      heading1("2. Architecture du système"),
      heading2("2.1 Vue d'ensemble"),
      para("Le système repose sur une architecture client-serveur centralisée. Le serveur joue le rôle d'intermédiaire unique : il maintient la table des clients connectés, les paires de jumeaux, et propage chaque changement d'état à l'appareil apparié."),
      spacer(),
      makeTable(
        ["Composant", "Rôle", "Technologie"],
        [
          ["Serveur (PC 1)", "Autorité centrale, gestion des threads et des fichiers", "C / pthreads / TCP"],
          ["Client PC (PC 2)", "Client interactif en ligne de commande", "C / TCP"],
          ["Client Android", "Client interactif via Termux", "C / TCP"],
          ["Client iPhone", "Client Python léger", "Python 3 / TCP"],
        ],
        [2500, 4000, 2500]
      ),
      spacer(),
      heading2("2.2 Protocole de messages"),
      para("Les échanges suivent un protocole texte simple, une commande par ligne, inspiré des protocoles comme SMTP ou Redis."),
      spacer(),
      makeTable(
        ["Requête (client → serveur)", "Description", "Rôle requis"],
        [
          ["CONNECT [user] [pass]", "Authentification et enregistrement", "tous"],
          ["STATE ON|OFF", "Changer l'état + notifier le jumeau", "user"],
          ["SYNC [id_jumeau]", "Créer une paire de jumeaux", "user"],
          ["PING", "Vérifier la connexion", "tous"],
          ["LIST", "Lister tous les clients", "admin"],
          ["QUIT", "Se déconnecter proprement", "tous"],
        ],
        [3000, 4000, 2000]
      ),
      spacer(),
      makeTable(
        ["Réponse (serveur → client)", "Signification"],
        [
          ["OK [message]", "Opération réussie"],
          ["UPDATE [state] from=[id]", "Propagation d'un changement d'état (jumeau)"],
          ["PONG", "Réponse à PING"],
          ["ERROR [message]", "Erreur (authentification, droits, syntaxe…)"],
          ["CLIENTS [liste]", "Réponse à LIST (admin)"],
        ],
        [3500, 5500]
      ),
      spacer(),
      heading2("2.3 Structure des fichiers de données"),
      heading3("clients.txt"),
      codeBlock("ID Etat Statut"),
      codeBlock("1 ON connecte"),
      codeBlock("2 OFF connecte"),
      spacer(),
      heading3("pairs.txt"),
      codeBlock("Client1 Client2"),
      codeBlock("1 2"),
      spacer(),
      heading3("histo.txt"),
      codeBlock("2025-06-01 14:32:01 | 1 | CONNECT | alice | succes"),
      codeBlock("2025-06-01 14:32:15 | 1 | STATE   | ON    | succes"),
      codeBlock("2025-06-01 14:32:15 | 2 | UPDATE  | ON    | recu"),
      spacer(),
      heading3("credentials.txt"),
      codeBlock("admin secret123 admin"),
      codeBlock("alice pass123 user"),
      codeBlock("bob   pass456 user"),
      spacer(),

      /* ══════════════ 3. MÉTHODOLOGIE ══════════════ */
      pageBreak(),
      heading1("3. Méthodologie de développement"),
      heading2("3.1 Approche adoptée"),
      para("Nous avons suivi une démarche incrémentale en respectant l'ordre recommandé par le cahier des charges : fichiers, TCP séquentiel, opérations métier, parallélisme, puis bonus. Chaque étape a été validée par des tests avant de passer à la suivante."),
      heading2("3.2 Serveur parallèle"),
      para("Le passage du mode séquentiel au mode parallèle a constitué la principale difficulté technique. Le serveur crée un thread POSIX par connexion entrante avec pthread_create(). Le thread est détaché (pthread_detach) afin que ses ressources soient libérées automatiquement à sa terminaison, sans appel à pthread_join()."),
      para("Trois mutex distincts protègent les accès concurrents : un pour chaque fichier (clients.txt, pairs.txt, histo.txt) et un quatrième pour la table en mémoire. Cette granularité réduit la contention par rapport à un verrou global unique."),
      codeBlock("pthread_mutex_lock(&mtx_clients);"),
      codeBlock("FILE *f = fopen(FILE_CLIENTS, \"w\");"),
      codeBlock("/* ... écriture ... */"),
      codeBlock("pthread_mutex_unlock(&mtx_clients);"),
      spacer(),
      heading2("3.3 Persistance et rechargement"),
      para("Les données sont sauvegardées dans des fichiers texte simples, ce qui facilite l'inspection manuelle et le débogage. Chaque écriture passe par la fonction save_clients() qui verrouille le mutex, ouvre le fichier en mode w (réécriture complète) et libère le verrou. Cette approche garantit la cohérence : il n'y a jamais de lignes partiellement écrites."),
      para("Au démarrage, load_clients() recharge la table depuis clients.txt, permettant de reprendre la session précédente même après un arrêt ou un crash du serveur."),
      spacer(),

      /* ══════════════ 4. BONUS ══════════════ */
      heading1("4. Fonctionnalités bonus"),
      heading2("4.1 Watchdog — haute disponibilité"),
      para("Le mécanisme de watchdog garantit que le serveur reste disponible même en cas de crash inattendu (segmentation fault, signal externe, etc.)."),
      para("Implémentation : à l'entrée de main(), un processus père appelle fork(). Le fils exécute le serveur réel (run_tcp ou run_udp). Le père entre dans une boucle waitpid() bloquante. Si le fils se termine anormalement (WIFSIGNALED), le père attend 2 secondes et le redémarre. Les relances sont journalisées dans data/watchdog.log avec un horodatage."),
      codeBlock("pid_t pid = fork();"),
      codeBlock("if (pid == 0) { run_tcp(port); exit(0); }"),
      codeBlock("waitpid(pid, &status, 0);"),
      codeBlock("if (WIFSIGNALED(status)) { sleep(2); /* redémarrer */ }"),
      spacer(),
      heading2("4.2 Mode UDP"),
      para("Le serveur accepte un argument --udp en ligne de commande. En mode UDP, les échanges sont sans connexion : le serveur appelle recvfrom() et répond avec sendto(). Les commandes supportées en UDP sont PING/PONG et STATE. Le client C et le client Python supportent également le mode UDP."),
      heading2("4.3 Profils admin / utilisateur"),
      para("Un fichier credentials.txt définit les identifiants et le rôle de chaque utilisateur. À la réception de CONNECT [user] [pass], le serveur authentifie l'utilisateur et lui attribue le rôle correspondant. La commande LIST, qui retourne la liste de tous les clients, est réservée aux administrateurs. Toute tentative d'un utilisateur standard reçoit la réponse ERROR Accès refusé (admin requis)."),
      spacer(),

      /* ══════════════ 5. DIFFICULTÉS ══════════════ */
      pageBreak(),
      heading1("5. Difficultés rencontrées"),
      heading2("5.1 Synchronisation des threads"),
      para("La première difficulté a été de déterminer le bon niveau de granularité pour les mutex. Un verrou global unique simplifiait le code mais bloquait tous les threads même pour des opérations sur des fichiers différents. Nous avons opté pour un mutex par fichier plus un mutex pour la table en mémoire, ce qui est un bon compromis entre sécurité et performance."),
      heading2("5.2 Propagation de l'état au jumeau"),
      para("Envoyer un UPDATE au jumeau depuis le thread du client émetteur pose un problème : la socket du jumeau est gérée par un autre thread. La solution adoptée consiste à stocker le descripteur de socket (sockfd) dans la structure Client en mémoire. Lorsqu'un STATE est reçu, le serveur appelle send_to_client(twin_id, ...) qui récupère le sockfd du jumeau depuis la table partagée (protégée par mtx_mem)."),
      heading2("5.3 Compatibilité iOS"),
      para("La compilation de code C sur iPhone via iSH s'est révélée fragile (bibliothèque pthread non disponible sur musl Alpine dans certaines versions). Nous avons donc fourni un client Python alternatif qui reproduit le même comportement sans dépendances système particulières, ce qui fonctionne sur iOS via l'application iSH ou Pythonista."),
      heading2("5.4 Détection de déconnexion"),
      para("recv() retourne 0 lorsque la connexion est fermée proprement et -1 en cas d'erreur réseau. Les deux cas déclenchent le même comportement : marquage du client comme déconnecté, fermeture de la socket et terminaison du thread."),
      spacer(),

      /* ══════════════ 6. ÉTAT DU PROJET ══════════════ */
      heading1("6. État actuel du projet"),
      heading2("6.1 Fonctionnalités réalisées"),
      bullet("Connexion TCP entre au moins 3 machines simultanées"),
      bullet("Serveur parallèle : un thread pthread par client"),
      bullet("Persistance complète : clients.txt, pairs.txt, histo.txt"),
      bullet("Rechargement des données au redémarrage"),
      bullet("Synchronisation en temps réel des jumeaux numériques"),
      bullet("Authentification par identifiants (credentials.txt)"),
      bullet("Profil admin (commande LIST) et profil utilisateur"),
      bullet("Mode UDP via argument --udp"),
      bullet("Watchdog automatique avec journal watchdog.log"),
      bullet("Client C multiplateforme (Linux, Termux/Android)"),
      bullet("Client Python pour iOS (Pythonista / iSH)"),
      spacer(),
      heading2("6.2 Limitations connues"),
      bullet("La table clients est limitée à 64 entrées simultanées (constante MAX_CLIENTS)"),
      bullet("Le rechargement ne restaure pas les sockets des sessions précédentes (normal : les sockets sont éphémères)"),
      bullet("Le mode UDP ne supporte pas les opérations SYNC (sans état côté serveur, l'appariement persistant n'est pas géré)"),
      spacer(),

      /* ══════════════ 7. BILAN ══════════════ */
      heading1("7. Bilan"),
      para("Ce projet nous a permis d'appréhender concrètement les défis des systèmes distribués : gestion de la concurrence, cohérence des données partagées, robustesse face aux pannes réseau et aux déconnexions inopinées."),
      para("La notion de watchdog, que nous avons ajoutée comme bonus sécurité, est directement inspirée des pratiques industrielles (systemd, supervisord, Kubernetes liveness probes). Son implémentation par fork() nous a amenés à comprendre en profondeur le modèle de processus Unix."),
      para("Sur le plan du travail en groupe, la répartition par couche fonctionnelle (réseau / concurrence / persistance / client) s'est avérée efficace. Les interfaces entre couches ont été définies au préalable (signatures des fonctions, format des fichiers) ce qui a limité les conflits lors de l'intégration."),
      spacer(),

      /* ══════════════ SOURCES ══════════════ */
      heading1("Sources"),
      bullet("W. Richard Stevens, Bill Fenner, Andrew M. Rudoff — Unix Network Programming, Vol. 1 (Prentice Hall)"),
      bullet("Manuel POSIX pthreads : https://man7.org/linux/man-pages/man7/pthreads.7.html"),
      bullet("RFC 793 — Transmission Control Protocol (TCP)"),
      bullet("Cours PSR — Mme Khaoula BEDOUI, ENICarthage 2025–2026"),
      bullet("Termux documentation : https://wiki.termux.com"),
      bullet("Grand View Research — Team Collaboration Software Market (contexte et enjeux)"),
    ]
  }]
});

Packer.toBuffer(doc).then(buf => {
  fs.writeFileSync('/home/claude/quantum_twin/docs/rapport_quantum_twin.docx', buf);
  console.log('Rapport généré : docs/rapport_quantum_twin.docx');
});
