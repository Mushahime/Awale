// client2.h
#ifndef CLIENT_H
#define CLIENT_H

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
#include "utilsClient.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "client2.h"
#include "../awale.h"
#include <sys/select.h>
#include <sys/time.h>
#include "commands.h"

// Constants
#define MAX_INPUT_LENGTH 256
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
#define CRLF     "\r\n"
#define BUF_SIZE 1024
#define BUF_SAVE_SIZE 3000
#define MAX_BIO_LENGTH 1000
#define PSEUDO_MAX_LENGTH 50
#define PSEUDO_MIN_LENGTH 2
#define MAX_PARTIES 25

// Typedefs
typedef enum
{
    SEND_PUBLIC_MESSAGE = 1,
    SEND_PRIVATE_MESSAGE,
    LIST_USERS,
    BIO_OPTIONS,
    PLAY_AWALE,
    LIST_GAMES_IN_PROGRESS,
    SEE_SAVE,
    SPEC,
    BLOCK,
    FRIEND,
    CLEAR_SCREEN,
    NOT_IMPLEMENTED,
    QUIT
} MenuChoice;

// Global
extern char pseudo[PSEUDO_MAX_LENGTH];
extern bool partie_en_cours;
extern char save[MAX_PARTIES][BUF_SAVE_SIZE+BUF_SIZE];
extern int save_count;
extern int save_index;

// Function Prototypes
void get_multiline_input(char *buffer, int max_size);
int get_valid_pseudo(SOCKET sock);
void display_menu();
void clear_screen_custom();
void clear_screen_custom2();
void app(const char *address, int port);
void handle_server_message(SOCKET sock, char *buffer);
void handle_user_input(SOCKET sock);

#endif /* guard */