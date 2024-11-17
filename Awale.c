#include "Awale.h"
#include <stdio.h>
#include <string.h> // For memcpy

// Initialize the game board
void initBoard(Game *game)
{
    game->scoreFirstPlayer = 0;
    game->scoreSecondPlayer = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
        game->board[i] = 4;

}

// Print the current state of the board with scores
void printBoard(const Game *game)
{
    // ANSI escape codes for colors
    const char *blue = "\033[1;34m";
    const char *green = "\033[1;32m";
    const char *reset = "\033[0m";

    printf("Score:\n");
    printf("%sPlayer 1: %d%s\n", blue, game->scoreFirstPlayer, reset);
    printf("%sPlayer 2: %d%s\n\n", green, game->scoreSecondPlayer, reset);

    printf("  Player 2\n");
    printf(" -----------------------------\n");
    printf("|");
    for (int i = BOARD_SIZE - 1; i >= BOARD_SIZE / 2; i--)
    {
        printf(" %2d |", game->board[i]);
    }
    printf("\n -----------------------------\n");

    printf("|");
    for (int i = 0; i < BOARD_SIZE / 2; i++)
    {
        printf(" %2d |", game->board[i]);
    }
    printf("\n -----------------------------\n");
    printf("  Player 1\n\n");
}

// Check if a player is in famine (no seeds on their side)
bool checkFamine(Game *game, int player)
{
    int start = (player == 1) ? 0 : BOARD_SIZE / 2;
    int end = start + BOARD_SIZE / 2;

    for (int i = start; i < end; i++)
    {
        if (game->board[i] > 0)
        {
            return false;
        }
    }
    return true;
}

// Check if the game has finished (both players are in famine)
bool gameFinished(Game *game)
{
    return checkFamine(game, 1) && checkFamine(game, 2);
}

// Distribute seeds from the selected pit and return the last pit index
int distributeSeeds(Game *game, int player, int seeds, int pit)
{
    game->board[pit] = 0; // Empty the selected pit
    int i = pit;
    int lastPit = -1;
    while (seeds > 0)
    {
        i = (i + 1) % BOARD_SIZE;
        if (i != pit) // Skip the original pit
        {
            game->board[i]++;
            seeds--;
            lastPit = i; // Update lastPit when a seed is placed
        }
    }
    return lastPit;
}

// Capture seeds from pits with exactly 2 or 3 seeds, starting from lastPit and moving backward
int captureSeeds(Game *game, int player, int lastPit)
{
    if (lastPit < 0 || lastPit >= BOARD_SIZE)
        return 0; // No capture possible if lastPit is invalid

    int capturedSeeds = 0;

    // Start capturing from the last pit and move backward
    for (int i = lastPit; i >= 0; i--)
    {
        if (game->board[i] == 2 || game->board[i] == 3)
        {
            capturedSeeds += game->board[i];
            game->board[i] = 0; // Empty the pit after capturing
        }
        else
        {
            break; // Stop capturing if the pit doesn't contain 2 or 3 seeds
        }
    }

    return capturedSeeds;
}

// Update the player's score with the number of captured seeds
void updateScore(Game *game, int player, int capturedSeeds)
{
    if (player == 1)
    {
        game->scoreFirstPlayer += capturedSeeds;
    }
    else
    {
        game->scoreSecondPlayer += capturedSeeds;
    }
}

// Check if making a move would cause the opponent to be in famine
bool canCauseFamine(Game *game, int player, int pit)
{
    int seeds = game->board[pit];
    if (seeds == 0)
        return false; // Can't cause famine if there are no seeds to play

    // Create a copy of the game state
    Game copyGame;
    memcpy(&copyGame, game, sizeof(Game));

    // Simulate the move
    int lastPit = distributeSeeds(&copyGame, player, seeds, pit);
    int capturedSeeds = captureSeeds(&copyGame, player, lastPit);
    updateScore(&copyGame, player, capturedSeeds);

    int opponent = (player == 1) ? 2 : 1;
    return checkFamine(&copyGame, opponent);
}

// Check if the player can make any move that does not cause opponent's famine
bool canFeedOpponent(Game *game, int player)
{
    int startPit = (player == 1) ? 0 : 6;
    int endPit = startPit + 6;
    int opponentStart = (player == 1) ? 6 : 0;
    int opponentEnd = opponentStart + 6;

    for (int pit = startPit; pit < endPit; pit++)
    {
        if (game->board[pit] > 0)
        {
            // Make a copy of the game to simulate the move
            Game copyGame;
            memcpy(&copyGame, game, sizeof(Game));

            // Simulate the move by distributing seeds
            int seeds = copyGame.board[pit];
            int lastPit = distributeSeeds(&copyGame, player, seeds, pit);

            // Capture seeds based on lastPit
            int capturedSeeds = captureSeeds(&copyGame, player, lastPit);
            updateScore(&copyGame, player, capturedSeeds);

            // Check if opponent has any seeds after the move
            bool opponentHasSeeds = false;
            for (int i = opponentStart; i < opponentEnd; i++)
            {
                if (copyGame.board[i] > 0)
                {
                    opponentHasSeeds = true;
                    break;
                }
            }

            if (opponentHasSeeds)
            {
                // Found a move that does not cause opponent's famine
                return true;
            }
        }
    }

    // No move found that does not cause opponent's famine
    return false;
}

// Execute a player's move
bool play(Game *game, int player, int pit)
{
    // First, check if the player can make a move that does not cause opponent's famine
    bool canFeed = canFeedOpponent(game, player);

    if (canFeed)
    {
        // The player must make a move that does not cause opponent's famine
        if (canCauseFamine(game, player, pit))
        {
            printf("Invalid move: Cannot cause opponent's famine. Choose another pit.\n");
            return false;
        }
    }
    // Check if the selected pit has seeds
    if (game->board[pit] == 0)
    {
        printf("Invalid move: Pit is empty. Choose another pit.\n");
        return false;
    }

    // Make the move
    int seeds = game->board[pit];
    int lastPit = distributeSeeds(game, player, seeds, pit);

    // Capture seeds based on lastPit
    int capturedSeeds = captureSeeds(game, player, lastPit);
    updateScore(game, player, capturedSeeds);

    // Check if opponent is in famine after the move
    int opponent = (player == 1) ? 2 : 1;
    if (checkFamine(game, opponent))
    {
        if (canFeed)
        {
            // Opponent is in famine but can be fed via other moves
            printf("Opponent is in famine but can be fed. Continue the game.\n");
        }
        else
        {
            // Opponent is in famine and cannot be fed, end the game
            // Collect remaining seeds
            for (int i = 0; i < BOARD_SIZE; i++)
            {
                if (player == 1)
                {
                    game->scoreFirstPlayer += game->board[i];
                }
                else
                {
                    game->scoreSecondPlayer += game->board[i];
                }
                game->board[i] = 0;
            }
            printf("Game over! Player %d has caused the game to end.\n", player);
        }
    }

    return true;
}

void startGame()
{
    Game game;
    initBoard(&game);

    int currentPlayer = 1;
    while (!gameFinished(&game))
    {
        printBoard(&game);

        int pit;
        printf("Player %d, choose a pit (%d-%d): ", currentPlayer, (currentPlayer - 1) * 6, currentPlayer * 6 - 1);
        if (scanf("%d", &pit) != 1)
        {
            printf("Invalid input. Please enter a number.\n");
            // Clear the input buffer
            while (getchar() != '\n')
                ;
            continue;
        }

        if ((currentPlayer == 1 && pit >= 0 && pit < 6) || (currentPlayer == 2 && pit >= 6 && pit < 12))
        {
            if (play(&game, currentPlayer, pit))
            {
                currentPlayer = (currentPlayer == 1) ? 2 : 1;
            }
            else
            {
                // Invalid move, prompt again
            }
        }
        else
        {
            printf("Invalid pit number. Try again.\n");
        }
    }

    // ANSI escape codes for colors
    const char *blue = "\033[1;34m";
    const char *green = "\033[1;32m";
    const char *reset = "\033[0m";

    printf("Game over!\n");
    printf("Final scores:\n");
    printf("%sPlayer 1: %d%s\n", blue, game.scoreFirstPlayer, reset);
    printf("%sPlayer 2: %d%s\n", green, game.scoreSecondPlayer, reset);

    if (game.scoreFirstPlayer > game.scoreSecondPlayer)
    {
        printf("%sPlayer 1 wins!%s\n", blue, reset);
    }
    else if (game.scoreSecondPlayer > game.scoreFirstPlayer)
    {
        printf("%sPlayer 2 wins!%s\n", green, reset);
    }
    else
    {
        printf("It's a tie!\n");
    }
}
