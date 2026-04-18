# ============================================================
#   QUANTUM TWIN — Makefile
#   Ecole Nationale d'Ingénieurs de Carthage — PSR 2025-2026
# ============================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -g -pthread
SRC     = src
BIN     = .

all: server client

server: $(SRC)/server.c
	$(CC) $(CFLAGS) -o $(BIN)/server $(SRC)/server.c -lpthread
	@echo "✓ server compilé"

client: $(SRC)/client.c
	$(CC) $(CFLAGS) -o $(BIN)/client $(SRC)/client.c -lpthread
	@echo "✓ client compilé"

init-data:
	@mkdir -p data
	@echo "ID Etat Statut"      >  data/clients.txt
	@echo "Client1 Client2"     >  data/pairs.txt
	@touch data/histo.txt
	@echo "admin  secret123  admin" >  data/credentials.txt
	@echo "alice  pass123    user"  >> data/credentials.txt
	@echo "bob    pass456    user"  >> data/credentials.txt
	@echo "✓ Fichiers de données initialisés dans data/"

clean:
	rm -f server client
	@echo "✓ Binaires supprimés"

clean-data:
	rm -f data/*.txt data/*.log
	@echo "✓ Données effacées"

.PHONY: all server client init-data clean clean-data
