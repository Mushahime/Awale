// server2.h
#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32
#include <winsock2.h>
#elif defined (linux)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <pthread.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#else
#error not defined for this platform
#endif

#define CRLF        "\r\n"
#define PORT        1977
#define MAX_CLIENTS 100
#define BUF_SIZE    1024
#define MAX_BIO_LENGTH 1000
#define PSEUDO_MAX_LENGTH 50
#define PSEUDO_MIN_LENGTH 2

extern pthread_mutex_t clients_mutex;
extern pthread_mutex_t socket_mutex;
#define MUTEX_TIMEOUT_SEC 10

typedef struct {
    SOCKET sock;
    char name[BUF_SIZE];
    char bio[MAX_BIO_LENGTH];
    int has_bio;
} Client;

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server);
static void remove_client(Client *clients, int to_remove, int *actual);
static void clear_clients(Client *clients, int actual);
static int check_pseudo(Client *clients, int actual, const char *pseudo);
static void handle_bio_command(Client *clients, int actual, int client_index, const char *buffer);
static void handle_private_message(Client *clients, int actual, int sender_index, const char *buffer);
static void list_connected_clients(Client *clients, int actual, int requester_index);
static void handle_awale_command(Client *clients, int actual, int client_index, const char *buffer);

#endif 