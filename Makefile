CC = gcc
CFLAGS = -Wall -Wextra -Wno-format-truncation -Wno-unused-parameter -Wno-missing-field-initializers -g -pthread

CLIENT_DIR = Client
SERVER_DIR = Serveur

all: server client awale

# Compilation du serveur avec awale.o
server: $(SERVER_DIR)/server2.c awale.o
	$(CC) $(CFLAGS) -o $(SERVER_DIR)/server $(SERVER_DIR)/server2.c awale.o

# Compilation du client avec awale.o
client: $(CLIENT_DIR)/client2.c awale.o
	$(CC) $(CFLAGS) -o $(CLIENT_DIR)/client $(CLIENT_DIR)/client2.c awale.o

# Compilation de awale
awale: awale.o main.o
	$(CC) $(CFLAGS) -o awale awale.o main.o

awale.o: awale.c awale.h
	$(CC) $(CFLAGS) -c awale.c

main.o: main.c awale.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(SERVER_DIR)/server $(CLIENT_DIR)/client awale *.o

.PHONY: all clean