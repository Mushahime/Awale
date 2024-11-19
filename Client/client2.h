// client2.h
#ifndef CLIENT_H
#define CLIENT_H

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
#include "utilsClient.h"
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
// Global Variables
char pseudo[PSEUDO_MAX_LENGTH];
bool partie_en_cours = false;
char save[MAX_PARTIES][BUF_SAVE_SIZE+BUF_SIZE];
int save_count = 0;
int save_index = 0;


// Function Prototypes
void get_multiline_input(char *buffer, int max_size);
int get_valid_pseudo(SOCKET sock);
void handle_send_public_message(SOCKET sock);
void handle_send_private_message(SOCKET sock);
void handle_list_users(SOCKET sock);
void handle_bio_options(SOCKET sock);
void handle_play_awale(SOCKET sock);
void handle_list_games(SOCKET sock);
void handle_quit();
void handle_user_input(SOCKET sock);
void handle_server_message(SOCKET sock, char *buffer);
void handle_save();
void demo_partie(const char *buffer);
void saver(SOCKET sock, char *buffer);
void display_menu();
void clear_screen_custom();
void clear_screen_custom2();
void app(const char *address, int port);
void process_awale_message(SOCKET sock, char *msg_body);
void process_error_message(SOCKET sock, char *buffer);
bool process_fight_message(SOCKET sock, char *buffer);
void process_game_over_message(SOCKET sock, char *buffer);
void process_private_message(char *buffer);
void process_system_message(char *buffer);
void process_challenge_message(char *buffer);
void prompt_for_move(SOCKET sock, int joueur, const char *nom, int plateau[], int score_joueur1, int score_joueur2);
void prompt_for_new_move(SOCKET sock, int joueur);
void handle_spec(SOCKET sock);
void handle_quit_game(SOCKET sock);
#endif /* guard */