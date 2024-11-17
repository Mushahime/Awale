// server2.h
#ifndef COMMANDS_H
#define COMMANDS_H

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
#include <stdlib.h>
#include "../awale.h"
#include "utilsServer.h"
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
#define INVALID_MOVE_FAMINE "The selected case would cause famine for your opponent"
#define INVALID_MOVE_EMPTY "The selected case is empty"

#else
#error not defined for this platform
#endif

void clean_invalid_parties(Client *clients, int actual);
int check_pseudo(Client *clients, int actual, const char *pseudo);
void list_connected_clients(Client *clients, int actual, int requester_index);
int find_challenge(const char *name);
void add_challenge(const char *challenger, const char *challenged);
void remove_challenge(int index);
void handle_awale_response(Client *clients, int actual, int client_index, const char *response);
void handle_awale_challenge(Client *clients, int actual, int client_index, const char *target_pseudo);
void handle_awale_move(Client *clients, int actual, int client_index, const char *move);
void handle_awale_list(Client *clients, int actual, int client_index);
void handle_bio_command(Client *clients, int actual, int client_index, const char *buffer);
void handle_private_message(Client *clients, int actual, int sender_index, const char *buffer);

void stream_move(SOCKET sock, const char *buffer, PartieAwale *partieAwale);
void addSpectator(PartieAwale *partieAwale, Client newSpectator);
void initSpectators(Client *clients, int actual, PartieAwale *partieAwale);
void allowAll(Client *clients, int actual, PartieAwale *partieAwale);
void clearSpectators(PartieAwale *PartieAwale);
/*
1-watch an awale game as an observator // ask the server to swend the board and who is playing
2-make game private // modify the game struct and add to it a list of clients that have permission
3-
3-sending messages during the game // add a menu inside all elements of the menu
*/

#endif