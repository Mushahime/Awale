CC = gcc
CFLAGS = -Wall -Wunused-variable -Wextra -Wno-format-truncation -Wno-unused-parameter -Wno-missing-field-initializers -g -pthread 

CLIENT_DIR = Client
SERVER_DIR = Serveur

all: server client awale

# Compilation du serveur avec utilsServer.o, commands.o, and awale.o
server: $(SERVER_DIR)/server2.c $(SERVER_DIR)/utilsServer.o $(SERVER_DIR)/commands.o awale.o
	$(CC) $(CFLAGS) -o $(SERVER_DIR)/server $(SERVER_DIR)/server2.c $(SERVER_DIR)/utilsServer.o $(SERVER_DIR)/commands.o awale.o

# Compilation du client with utilsClient.o and awale.o
client: $(CLIENT_DIR)/client2.c $(CLIENT_DIR)/utilsClient.o awale.o
	$(CC) $(CFLAGS) -o $(CLIENT_DIR)/client $(CLIENT_DIR)/client2.c $(CLIENT_DIR)/utilsClient.o awale.o

# Compilation de awale
awale: awale.o main.o
	$(CC) $(CFLAGS) -o awale awale.o main.o

# Object file for utilsServer.c
$(SERVER_DIR)/utilsServer.o: $(SERVER_DIR)/utilsServer.c $(SERVER_DIR)/utilsServer.h
	$(CC) $(CFLAGS) -c $(SERVER_DIR)/utilsServer.c -o $(SERVER_DIR)/utilsServer.o

# Object file for commands.c
$(SERVER_DIR)/commands.o: $(SERVER_DIR)/commands.c $(SERVER_DIR)/commands.h
	$(CC) $(CFLAGS) -c $(SERVER_DIR)/commands.c -o $(SERVER_DIR)/commands.o

# Object file for utilsClient.c
$(CLIENT_DIR)/utilsClient.o: $(CLIENT_DIR)/utilsClient.c $(CLIENT_DIR)/utilsClient.h
	$(CC) $(CFLAGS) -c $(CLIENT_DIR)/utilsClient.c -o $(CLIENT_DIR)/utilsClient.o

# Object file for awale.c
awale.o: awale.c awale.h
	$(CC) $(CFLAGS) -c awale.c

# Object file for main.c
main.o: main.c awale.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(SERVER_DIR)/server $(CLIENT_DIR)/client awale *.o $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o

.PHONY: all clean
