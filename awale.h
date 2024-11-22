#ifndef AWALE_H
#define AWALE_H

#define TAILLE_PLATEAU 12
#define CASES_PAR_JOUEUR 6
#define GRAINES_A_GAGNER 25

#include <stdbool.h>
#include <stdio.h>

// Struct for the Awale game
typedef struct
{
    int plateau[TAILLE_PLATEAU]; // Game board (12 cases)
    int score_joueur1;           // Score of player 1
    int score_joueur2;           // Score of player 2
} JeuAwale;

// Function to initialize the game board
void initialiser_plateau(JeuAwale *jeu);

// Function to display the game board
void afficher_plateau(JeuAwale *jeu);
void afficher_plateau2(int plateau[TAILLE_PLATEAU], int score_joueur1, int score_joueur2);

// Function to play a move
bool jouer_coup(JeuAwale *jeu, int joueur, int case_depart);

// Function to check if the game is over
bool verifier_fin_partie(JeuAwale *jeu);

// Function to check if a move is valid
void mettre_a_jour_scores(JeuAwale *jeu, int joueur, int graines);

#endif
