#ifndef AWALE_H
#define AWALE_H

#define TAILLE_PLATEAU 12
#define CASES_PAR_JOUEUR 6
#define GRAINES_A_GAGNER 25

#include <stdbool.h>
#include <stdio.h>

// Structure représentant l'état du jeu d'Awalé
typedef struct {
    int plateau[TAILLE_PLATEAU];
    int score_joueur1;
    int score_joueur2;
} JeuAwale;

// Fonction pour initialiser le plateau de jeu
void initialiser_plateau(JeuAwale *jeu);

// Fonction pour afficher l'état actuel du plateau
void afficher_plateau(JeuAwale *jeu);
void afficher_plateau2(int plateau[TAILLE_PLATEAU], int score_joueur1, int score_joueur2);

// Fonction pour jouer un coup dans le jeu d'Awalé
bool jouer_coup(JeuAwale *jeu, int joueur, int case_depart);

// Fonction pour vérifier si la partie est terminée
bool verifier_fin_partie(JeuAwale *jeu);

// Fonction pour gérer l'augmentation des scores
void mettre_a_jour_scores(JeuAwale *jeu, int joueur, int graines);

#endif
