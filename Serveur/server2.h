// server.h
#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#define BUF_SIZE 2048
#define MAX_CLIENTS 100
#define PORT 12346      // Port number

// Client states
#define STATE_IDLE 0
#define STATE_WAITING_FOR_ACCEPTANCE 1
#define STATE_BEING_INVITED 2
#define STATE_PLAYING 3

typedef struct Client {
    int sock;               // Client's socket
    char name[BUF_SIZE];    // Client's name
    int opponent_sock;      // Opponent's socket
    int state;              // Client's state
} Client;

void init_server(void);
void close_server(void);
void run_server(void);
int init_connection(void);
void close_connection(int sock);
int read_from_client(int sock, char *buffer);
void write_to_client(int sock, const char *buffer);

void handle_client_message(Client *clients, Client *sender, int *actual, const char *buffer);
void invite_player(Client *clients, Client *sender, int actual, const char *opponent_name);
void accept_invitation(Client *clients, Client *sender, int actual);
void refuse_invitation(Client *clients, Client *sender, int actual);
void start_game(Client *player1, Client *player2);
int random_choice();
Client *find_client_by_name(Client *clients, int actual, const char *name);
Client *find_client_by_sock(Client *clients, int actual, int sock);
void remove_client(Client *clients, int to_remove, int *actual);
void send_online_players(Client *clients, Client *sender, int actual);

#endif // SERVER_H
