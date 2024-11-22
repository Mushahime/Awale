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
#define RESERVED_WORDS_COUNT 18
#define _GNU_SOURCE // For strcasestr

// Typedefs
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

// Specific struct

// Structure to store a challenge (substrucutre of PartieAwale)
typedef struct
{
    char challenger[PSEUDO_MAX_LENGTH];                // pseudo of the challenger
    char challenged[PSEUDO_MAX_LENGTH];                // pseudo of the challenged
    bool prive;                                        // true if private challenge
    char private_spec[MAX_CLIENTS][PSEUDO_MAX_LENGTH]; // private spectators (who is allowed to see the game)
    int private_spec_count;                            // count of private spectators
    time_t challenge_time;                             // time of the challenge
} AwaleChallenge;

// Structure to store client data
typedef struct
{
    SOCKET sock;
    char name[BUF_SIZE];                         // pseudo
    char bio[MAX_BIO_LENGTH];                    // (for bio)
    int has_bio;                                 //  (for knowing if bio is set)
    int partie_index;                            // -1 if not in a game
    int point;                                   //(for ranking)
    char block[MAX_CLIENTS][PSEUDO_MAX_LENGTH];  // list of blocked users
    int nbBlock;                                 // number of blocked users
    char friend[MAX_CLIENTS][PSEUDO_MAX_LENGTH]; // list of friends
    int nbFriend;                                // number of friends

    bool has_pending_request;             // true if this client has a pending request
    char pending_from[PSEUDO_MAX_LENGTH]; // who sent the pending request
} Client;

// Structure to store a game
typedef struct
{
    AwaleChallenge awale_challenge; // instance of the challenge
    JeuAwale jeu;                   // instance of the game
    int tour;                       // to know who's turn it is
    Client Spectators[MAX_CLIENTS]; // spectators in the game
    int nbSpectators;               // number of spectators
    char cout[BUF_SAVE_SIZE];       // save of the game
    int cout_index;                 // index of the save
    bool in_save;                   // if the game is saved to know if we can join (for spectators)
} PartieAwale;

#else
#error Platform not supported
#endif

// Global
// extern pthread_mutex_t clients_mutex; //useless its asynchrone
// extern pthread_mutex_t socket_mutex;
extern AwaleChallenge awale_challenges[MAX_CHALLENGES];
extern int challenge_count;
extern PartieAwale awale_parties[MAX_PARTIES];
extern int partie_count;

static const char *RESERVED_WORDS[] = {
    "Private", "Public", "Challenge", "AWALE", "ERROR", "FAIL",
    "fight", "Spectator", "over", "expired", "spectating",
    "Friend", "declined", "score", "list", "save", "quit",
    "connected"};

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
void update_elo_ratings(Client *winner, Client *loser, bool isDraw);
void clean_invalid_parties(Client *clients, int actual);
int check_pseudo(Client *clients, int actual, const char *pseudo);
int find_challenge(const char *name);
void add_challenge(const char *challenger, const char *challenged, const char *message_rest);
void remove_challenge(int index);
void check_challenge_timeouts(Client *clients, int actual);
bool is_blocked_by(Client *clients, int actual, const char *sender, const char *recipient);

#endif /* UTILS_H */