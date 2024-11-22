CC = gcc
CFLAGS = -Wall -Wformat -Wdiscarded-qualifiers -Wimplicit-function-declaration -Wincompatible-pointer-types -Wunused-function -Wextra -Wno-format-truncation -Wno-unused-parameter -Wno-missing-field-initializers -Wno-unused-variable -g -pthread

CLIENT_DIR = Client
SERVER_DIR = Serveur

# Adding persistence objects
SERVER_OBJS = $(SERVER_DIR)/server2.o \
              $(SERVER_DIR)/utilsServer.o \
              $(SERVER_DIR)/commands.o \
              $(SERVER_DIR)/persistance.o \
              awale.o

CLIENT_OBJS = $(CLIENT_DIR)/client2.o \
              $(CLIENT_DIR)/utilsClient.o \
              $(CLIENT_DIR)/commands.o \
              awale.o

AWALE_OBJS = awale.o main.o

all: server client awale

# Rules for executables
server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_DIR)/server $(SERVER_OBJS) -lm

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_DIR)/client $(CLIENT_OBJS) -lm

awale: $(AWALE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# New rule for persistence
$(SERVER_DIR)/persistance.o: $(SERVER_DIR)/persistance.c $(SERVER_DIR)/persistance.h $(SERVER_DIR)/utilsServer.h
	$(CC) $(CFLAGS) -c $< -o $@

# Existing rules for server
$(SERVER_DIR)/server2.o: $(SERVER_DIR)/server2.c $(SERVER_DIR)/server2.h $(SERVER_DIR)/utilsServer.h $(SERVER_DIR)/commands.h $(SERVER_DIR)/persistance.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/utilsServer.o: $(SERVER_DIR)/utilsServer.c $(SERVER_DIR)/utilsServer.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

$(SERVER_DIR)/commands.o: $(SERVER_DIR)/commands.c $(SERVER_DIR)/commands.h $(SERVER_DIR)/utilsServer.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rules for client objects
$(CLIENT_DIR)/client2.o: $(CLIENT_DIR)/client2.c $(CLIENT_DIR)/client2.h $(CLIENT_DIR)/utilsClient.h $(CLIENT_DIR)/commands.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_DIR)/commands.o: $(CLIENT_DIR)/commands.c $(CLIENT_DIR)/client2.h $(CLIENT_DIR)/utilsClient.h $(CLIENT_DIR)/commands.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

$(CLIENT_DIR)/utilsClient.o: $(CLIENT_DIR)/utilsClient.c $(CLIENT_DIR)/utilsClient.h $(CLIENT_DIR)/commands.h awale.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rules for awale objects
awale.o: awale.c awale.h
	$(CC) $(CFLAGS) -c $< -o $@

main.o: main.c awale.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_DIR)/server $(CLIENT_DIR)/client awale
	rm -f *.o $(SERVER_DIR)/*.o $(CLIENT_DIR)/*.o
	rm -f client_data.txt

.PHONY: all clean