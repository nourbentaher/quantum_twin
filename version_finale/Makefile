CC      = gcc
CFLAGS  = -Wall -g -lpthread
SRC_DIR = src

all: server client

server:
	$(CC) $(CFLAGS) -o server $(SRC_DIR)/server.c -lpthread

client:
	$(CC) $(CFLAGS) -o client $(SRC_DIR)/client.c -lpthread

clean:
	rm -f server client

init-data:
	mkdir -p data
	@if [ ! -f data/credentials.txt ]; then \
		echo "admin secret123 admin" > data/credentials.txt; \
		echo "alice pass123 user" >> data/credentials.txt; \
		echo "bob pass456 user" >> data/credentials.txt; \
	fi
	@if [ ! -f data/clients.txt ]; then echo "ID Etat Statut" > data/clients.txt; fi
	@if [ ! -f data/pairs.txt ]; then echo "Client1 Client2" > data/pairs.txt; fi
	@touch data/histo.txt data/watchdog.log
	@echo "Données initialisées."

.PHONY: all clean init-data
