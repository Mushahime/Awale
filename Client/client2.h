// client.h
#ifndef CLIENT_H
#define CLIENT_H

#define BUF_SIZE 1024
#define PORT 12346       // Port number
#define STATE_IDLE 0

typedef struct Client {
    int sock;               // Client's socket
    char name[BUF_SIZE];    // Client's name
    int opponent_sock;      // Opponent's socket
    int state;              // Client's state
} Client;
void init_client(void);
void close_client(void);
void run_client(const char *address, const char *name);
 int init_connection(const char *address);
void close_connection(int sock);
int read_from_server(int sock, char *buffer);
void write_to_server(int sock, const char *buffer);

#endif // CLIENT_H
