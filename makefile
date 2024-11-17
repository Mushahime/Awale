CC = gcc
CFLAGS = -Wall -Wunused-variable -Wextra -Wno-format-truncation -Wno-unused-parameter -Wno-missing-field-initializers -g -pthread 

CLIENT_DIR = Client
SERVER_DIR = Serveur

all: server client Awale

# Compilation du serveur avec awale.o
server: $(SERVER_DIR)/server2.c Awale.o
	$(CC) $(CFLAGS) -o $(SERVER_DIR)/server $(SERVER_DIR)/server2.c Awale.o

# Compilation du client avec awale.o
client: $(CLIENT_DIR)/client2.c Awale.o
	$(CC) $(CFLAGS) -o $(CLIENT_DIR)/client $(CLIENT_DIR)/client2.c Awale.o

# Compilation de awale
Awale: Awale.o main.o
	$(CC) $(CFLAGS) -o Awale Awale.o main.o

Awale.o: Awale.c Awale.h
	$(CC) $(CFLAGS) -c Awale.c

main.o: main.c Awale.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(SERVER_DIR)/server $(CLIENT_DIR)/client Awale *.o

.PHONY: all clean