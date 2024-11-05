# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Wno-format-truncation -g -pthread

# Target executable names
TARGET_AWALE = awale
TARGET_CLIENT = client
TARGET_SERVER = server

# Object files for each target
OBJS_AWALE = awale.o main.o
OBJS_CLIENT = client.o
OBJS_SERVER = server.o

# Default target
all: $(TARGET_AWALE) $(TARGET_CLIENT) $(TARGET_SERVER)

# Link the target executable for awale
$(TARGET_AWALE): $(OBJS_AWALE)
	$(CC) $(OBJS_AWALE) -o $(TARGET_AWALE)

# Compile source files to object files for awale
awale.o: awale.c awale.h
	$(CC) $(CFLAGS) -c awale.c

main.o: main.c awale.h
	$(CC) $(CFLAGS) -c main.c

# Link the target executable for client
$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(OBJS_CLIENT) -o $(TARGET_CLIENT)

# Compile source files to object files for client
client.o: client.c
	$(CC) $(CFLAGS) -c client.c

# Link the target executable for server
$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(OBJS_SERVER) -o $(TARGET_SERVER)

# Compile source files to object files for server
server.o: server.c
	$(CC) $(CFLAGS) -c server.c

# Clean up built files
clean:
	rm -f $(OBJS_AWALE) $(OBJS_CLIENT) $(OBJS_SERVER) $(TARGET_AWALE) $(TARGET_CLIENT) $(TARGET_SERVER)

# Clean and rebuild
rebuild: clean all

# Prevent make from doing something with files named clean, all, or rebuild
.PHONY: clean all rebuild
