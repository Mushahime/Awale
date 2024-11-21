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
#include "commands.h"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
#define INVALID_MOVE_FAMINE "The selected case would cause famine for your opponent"
#define INVALID_MOVE_EMPTY "The selected case is empty"

typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
void app(int port);
#else
#error not defined for this platform
#endif



#endif