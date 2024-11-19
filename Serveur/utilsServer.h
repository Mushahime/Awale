#ifndef UTILS_H
#define UTILS_H

#ifdef WIN32
#include <winsock2.h>
#elif defined(__linux__) || defined(linux) || defined(__linux)

// Includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "../awale.h"

// Constants
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
#define CRLF "\r\n"
#define MAX_CLIENTS 100
#define BUF_SIZE 1024
#define MAX_BIO_LENGTH 1000
#define PSEUDO_MAX_LENGTH 50
#define PSEUDO_MIN_LENGTH 2
#define MAX_CHALLENGES 25
#define MAX_PARTIES 25
#define BUF_SAVE_SIZE 3000
#define MUTEX_TIMEOUT_SEC 10

// Typedefs
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

typedef struct {
    char challenger[PSEUDO_MAX_LENGTH]; // pseudo of the challenger
    char challenged[PSEUDO_MAX_LENGTH]; // pseudo of the challenged
    bool prive; // true if private challenge
    char private_spec[MAX_CLIENTS][PSEUDO_MAX_LENGTH]; // private spectators (who is allowed to see the game)
    int private_spec_count; // count of private spectators
} AwaleChallenge;

typedef struct {
    SOCKET sock;
    char name[BUF_SIZE]; // pseudo
    char bio[MAX_BIO_LENGTH]; // mutex maybe (for bio)
    int has_bio; // mutex maybe (for knowing if bio is set)
    int partie_index; // -1 if not in a game (mutex maybe)
    int point; // mutex maybe (for ranking)
} Client;

typedef struct {
    AwaleChallenge awale_challenge; // instance of the challenge
    JeuAwale jeu; // instance of the game
    int tour; // to know who's turn it is
    Client Spectators[MAX_CLIENTS]; // mutex maybe // spectators in the game
    int nbSpectators; // mutex maybe // number of spectators
    char cout[BUF_SAVE_SIZE]; // save of the game
    int cout_index; // index of the save
    bool in_save; // if the game is saved to know if we can join (for spectators)
} PartieAwale;

#else
#error Platform not supported
#endif

// Global
extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t socket_mutex;
extern AwaleChallenge awale_challenges[MAX_CHALLENGES]; // mutex maybe
extern int challenge_count; // mutex maybe
extern PartieAwale awale_parties[MAX_PARTIES]; // mutex maybe
extern int partie_count; // mutex maybe

// function declarations
void init(void);
void end(void);
int init_connection(int port);
void end_connection(int sock);
void remove_partie(int index, Client *clients);
Client *findClientByPseudo(Client *clients, int actual, const char *name);
void clear_clients(Client *clients, int actual);
void remove_client(Client *clients, int to_remove, int *actual);
int read_client(SOCKET sock, char *buffer);
void write_client(SOCKET sock, const char *buffer);
void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server);
void remove_spec(Client *clients, int to_remove, int actual, PartieAwale *partie);

#endif /* UTILS_H */