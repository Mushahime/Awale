CC = gcc
CFLAGS = -Wall -Wimplicit-function-declaration -Wincompatible-pointer-types -Wunused-function -Wextra -Wno-format-truncation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-variable -g -pthread -lm 

CLIENT_DIR = Client
SERVER_DIR = Serveur

# Définition des objets pour chaque exécutable
SERVER_OBJS = $(SERVER_DIR)/server2.o \
              $(SERVER_DIR)/utilsServer.o \
              $(SERVER_DIR)/commands.o \
              awale.o

CLIENT_OBJS = $(CLIENT_DIR)/client2.o \
              $(CLIENT_DIR)/utilsClient.o \
              awale.o

AWALE_OBJS = awale.o main.o

all: server client awale

# Règles de compilation des exécutables
server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_DIR)/server $(SERVER_OBJS) -lm

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_DIR)/client $(CLIENT_OBJS) -lm

awale: $(AWALE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Règles de compilation des objets du serveur
$(SERVER_DIR)/server2.o: $(SERVER_DIR)/server2.c $(SERVER_DIR)/server2.h $(SERVER_DIR)/utilsServer.h $(SERVER_DIR)/commands.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/utilsServer.o: $(SERVER_DIR)/utilsServer.c $(SERVER_DIR)/utilsServer.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/commands.o: $(SERVER_DIR)/commands.c $(SERVER_DIR)/commands.h $(SERVER_DIR)/utilsServer.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

# Règles de compilation des objets du client
$(CLIENT_DIR)/client2.o: $(CLIENT_DIR)/client2.c $(CLIENT_DIR)/client2.h $(CLIENT_DIR)/utilsClient.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_DIR)/utilsClient.o: $(CLIENT_DIR)/utilsClient.c $(CLIENT_DIR)/utilsClient.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

# Règles de compilation des objets awale
awale.o: awale.c awale.h
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c awale.h
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -f $(SERVER_DIR)/server $(CLIENT_DIR)/client awale
	rm -f *.o $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o

.PHONY: all clean