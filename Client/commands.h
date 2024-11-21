//commands.h
#ifndef COMMANDS_H
#define COMMANDS_H

// Includes
#include "client2.h"
#include "utilsClient.h"

// Function declarations
void handle_send_public_message(SOCKET sock);
void handle_send_private_message(SOCKET sock);
void handle_list_users(SOCKET sock);
void handle_bio_options(SOCKET sock);
void handle_play_awale(SOCKET sock);
void handle_list_games(SOCKET sock);
void handle_quit();
void handle_save();
void handle_spec(SOCKET sock);
void handle_quit_game(SOCKET sock);
void handle_block(SOCKET sock);
void handle_friend(SOCKET sock);

void demo_partie(const char *buffer);
void saver(SOCKET sock, char *buffer);
void process_awale_message(SOCKET sock, char *msg_body);
void process_error_message(SOCKET sock, char *buffer);
bool process_fight_message(SOCKET sock, char *buffer);
void process_game_over_message(SOCKET sock, char *buffer);
void process_private_message(char *buffer);
void process_system_message(char *buffer);
void process_challenge_message(char *buffer);
void prompt_for_move(SOCKET sock, int joueur, const char *nom, int plateau[], int score_joueur1, int score_joueur2);
void prompt_for_new_move(SOCKET sock, int joueur);

#endif /* guard */