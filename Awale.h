#ifndef AWALE_H
#define AWALE_H

#include <stdbool.h>
#include <stdio.h>
#define BOARD_SIZE 12

// Structure representing the game state
typedef struct {
    int board[BOARD_SIZE];
    int scoreFirstPlayer;
    int scoreSecondPlayer;
} Game;

// Function prototypes
void initBoard(Game *game);
void printBoard(const Game *game);
bool checkFamine(Game *game, int player);
bool gameFinished(Game *game);
int distributeSeeds(Game *game, int player, int seeds, int pit);
int captureSeeds(Game *game, int player, int lastPit);
void updateScore(Game *game, int player, int capturedSeeds);
bool canCauseFamine(Game *game, int player, int pit);
bool feedOpponent(Game *game, int player);
bool play(Game *game, int player, int pit);
void startGame();

#endif // AWALE_H
