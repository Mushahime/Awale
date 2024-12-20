// client2.h
#ifndef UTILS_CLIENT_H
#define UTILS_CLIENT_H

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
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>

// Constants
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
#define CRLF "\r\n"
#define BUF_SIZE 1024
#define MAX_BIO_LENGTH 1000
#define PSEUDO_MAX_LENGTH 50
#define PSEUDO_MIN_LENGTH 2

// Typedefs
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else
#error not defined for this platform
#endif

// Function declarations
void init(void);
void end(void);
int init_connection(const char *address, int port);
void end_connection(int sock);
int read_server(SOCKET sock, char *buffer);
void write_server(SOCKET sock, const char *buffer);
void clear_screen(void);

#endif /* guard */