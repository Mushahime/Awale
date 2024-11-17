// server2.h
#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32
#include <winsock2.h>
#elif defined(__linux__) || defined(linux) || defined(__linux)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h>  /* gethostbyname */
#include <pthread.h>
#include "utilsServer.h"
#include "../awale.h"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
#define INVALID_MOVE_FAMINE "The selected case would cause famine for your opponent"
#define INVALID_MOVE_EMPTY "The selected case is empty"

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#else
#error not defined for this platform
#endif

static int check_pseudo(Client *clients, int actual, const char *pseudo);
static void list_connected_clients(Client *clients, int actual, int requester_index);
static int find_challenge(const char *name);
static void add_challenge(const char *challenger, const char *challenged);
static void remove_challenge(int index);
static void handle_awale_response(Client *clients, int actual, int client_index, const char *response);
static void handle_awale_challenge(Client *clients, int actual, int client_index, const char *target_pseudo);
static void handle_awale_move(Client *clients, int actual, int client_index, const char *move);
static void handle_awale_list(Client *clients, int actual, int client_index);
static void handle_bio_command(Client *clients, int actual, int client_index, const char *buffer);
static void handle_private_message(Client *clients, int actual, int sender_index, const char *buffer);

static void app(void);

#endif