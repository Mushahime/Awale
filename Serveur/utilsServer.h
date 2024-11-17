#ifndef UTILS_H
#define UTILS_H

#ifdef WIN32
#include <winsock2.h>
#elif defined(__linux__) || defined(linux) || defined(__linux)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>

#include "../awale.h"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#else
#error Platform not supported
#endif

#define CRLF "\r\n"
#define PORT 1977
#define MAX_CLIENTS 100
#define BUF_SIZE 1024
#define MAX_BIO_LENGTH 1000
#define PSEUDO_MAX_LENGTH 50
#define PSEUDO_MIN_LENGTH 2

#define MUTEX_TIMEOUT_SEC 10

// Awale Challenge structure
typedef struct
{
    char challenger[PSEUDO_MAX_LENGTH];
    char challenged[PSEUDO_MAX_LENGTH];
} AwaleChallenge;

// Partie Awale structure, representing an Awale game instance
typedef struct
{
    AwaleChallenge awale_challenge;
    JeuAwale jeu;
    int tour;   // 1 for player1, 2 for player2
    bool prive; // true if the game is private
    Client *spectators;
    int nbSpectators;
} PartieAwale;

// Client structure representing each connected client
typedef struct
{
    SOCKET sock;
    char name[BUF_SIZE];
    char bio[MAX_BIO_LENGTH];
    int has_bio;
    int partie_index; // Current game index (-1 if none)
} Client;

extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t socket_mutex;

#define MAX_CHALLENGES 25
#define MAX_PARTIES 25

static AwaleChallenge awale_challenges[MAX_CHALLENGES];
static int challenge_count = 0;
static PartieAwale awale_parties[MAX_PARTIES];
static int partie_count = 0;

#ifdef __cplusplus
extern "C"
{
#endif

    // Function prototypes for connection and client handling
    void init(void);
    void end(void);
    int init_connection(void);
    void end_connection(int sock);
    void remove_partie(int index);

    void clear_clients(Client *clients, int actual);
    void remove_client(Client *clients, int to_remove, int *actual);
    int read_client(SOCKET sock, char *buffer);
    void write_client(SOCKET sock, const char *buffer);
    void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */
