#include "awale.h"

void initialiser_plateau(JeuAwale *jeu)
{
    for (int i = 0; i < TAILLE_PLATEAU; i++)
    {
        jeu->plateau[i] = 4;
    }
    jeu->score_joueur1 = 0;
    jeu->score_joueur2 = 0;
}

void afficher_plateau(JeuAwale *jeu)
{
    printf("\nJoueur 2:\n");
    printf("Cases :  11  10   9   8   7   6\n");
    printf("       [%2d][%2d][%2d][%2d][%2d][%2d]\n",
           jeu->plateau[11], jeu->plateau[10], jeu->plateau[9],
           jeu->plateau[8], jeu->plateau[7], jeu->plateau[6]);
    printf("       [%2d][%2d][%2d][%2d][%2d][%2d]\n",
           jeu->plateau[0], jeu->plateau[1], jeu->plateau[2],
           jeu->plateau[3], jeu->plateau[4], jeu->plateau[5]);
    printf("Cases :   0   1   2   3   4   5\n");
    printf("Joueur 1\n");
    printf("\nScore\nJ1: %d\nJ2: %d\n", jeu->score_joueur1, jeu->score_joueur2);
}

void afficher_plateau2(int plateau[TAILLE_PLATEAU], int score_joueur1, int score_joueur2)
{
    printf("\nJoueur 2:\n");
    printf("Cases :  11  10   9   8   7   6\n");
    printf("       [%2d][%2d][%2d][%2d][%2d][%2d]\n",
           plateau[11], plateau[10], plateau[9],
           plateau[8], plateau[7], plateau[6]);
    printf("       [%2d][%2d][%2d][%2d][%2d][%2d]\n",
           plateau[0], plateau[1], plateau[2],
           plateau[3], plateau[4], plateau[5]);
    printf("Cases :   0   1   2   3   4   5\n");
    printf("Joueur 1\n");
    printf("\nScore\nJ1: %d\nJ2: %d\n", score_joueur1, score_joueur2);
}

bool jouer_coup(JeuAwale *jeu, int joueur, int case_depart)
{
    if ((joueur == 1 && (case_depart < 0 || case_depart >= 6)) ||
        (joueur == 2 && (case_depart < 6 || case_depart >= 12)) ||
        jeu->plateau[case_depart] == 0)
    {
        return false;
    }

    int plateau_initial[TAILLE_PLATEAU];
    for (int i = 0; i < TAILLE_PLATEAU; i++)
    {
        plateau_initial[i] = jeu->plateau[i];
    }
    int score_initial_j1 = jeu->score_joueur1;
    int score_initial_j2 = jeu->score_joueur2;

    int graines = jeu->plateau[case_depart];
    jeu->plateau[case_depart] = 0;
    int position = case_depart;
    int positions_modifiees[TAILLE_PLATEAU] = {0};

    while (graines > 0)
    {
        position = (position + 1) % TAILLE_PLATEAU;
        if (position != case_depart)
        {
            jeu->plateau[position]++;
            positions_modifiees[position] = 1;
            graines--;
        }
    }

    bool famine_adversaire = true;
    bool famine_joueur = true;
    if (joueur == 1)
    {
        for (int i = 6; i < 12; i++)
        {
            if (jeu->plateau[i] > 0)
            {
                famine_adversaire = false;
                break;
            }
        }
        for (int i = 0; i < 6; i++)
        {
            if (jeu->plateau[i] > 0)
            {
                famine_joueur = false;
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < 6; i++)
        {
            if (jeu->plateau[i] > 0)
            {
                famine_adversaire = false;
                break;
            }
        }
        for (int i = 6; i < 12; i++)
        {
            if (jeu->plateau[i] > 0)
            {
                famine_joueur = false;
                break;
            }
        }
    }

    if (famine_adversaire)
    {
        for (int i = 0; i < TAILLE_PLATEAU; i++)
        {
            jeu->plateau[i] = plateau_initial[i];
        }
        jeu->score_joueur1 = score_initial_j1;
        jeu->score_joueur2 = score_initial_j2;
        printf("Coup invalide. L'adversaire ne doit pas avoir de famine.\n");
        return false;
    }

    if (famine_joueur)
    {
        for (int i = 0; i < TAILLE_PLATEAU; i++)
        {
            if (joueur == 1)
            {
                jeu->score_joueur2 += jeu->plateau[i];
            }
            else
            {
                jeu->score_joueur1 += jeu->plateau[i];
            }
            jeu->plateau[i] = 0;
        }
        return true;
    }

    int score = 0;
    if (joueur == 1)
    {
        for (int i = 6; i < 12; i++)
        {
            if (positions_modifiees[i] && (jeu->plateau[i] == 2 || jeu->plateau[i] == 3))
            {
                score += jeu->plateau[i];
                jeu->plateau[i] = 0;
            }
        }
        if (score > 0 && score < 25)
        {
            jeu->score_joueur1 += score;
        }
    }
    else
    {
        for (int i = 0; i < 6; i++)
        {
            if (positions_modifiees[i] && (jeu->plateau[i] == 2 || jeu->plateau[i] == 3))
            {
                score += jeu->plateau[i];
                jeu->plateau[i] = 0;
            }
        }
        if (score > 0 && score < 25)
        {
            jeu->score_joueur2 += score;
        }
    }

    return true;
}

bool nourrir_adversaire_possible(JeuAwale *jeu, int joueur)
{
    int debut = (joueur == 1) ? 0 : 6;
    int fin = debut + 6;

    for (int i = debut; i < fin; i++)
    {
        if (jeu->plateau[i] > 0)
        {
            int graines = jeu->plateau[i];
            jeu->plateau[i] = 0;
            int pos = i;

            // Simulation du mouvement
            for (int j = 0; j < graines; j++)
            {
                pos = (pos + 1) % 12;
                jeu->plateau[pos]++;
            }

            // Vérifier si le mouvement nourrit l'adversaire
            bool adversaire_nourri = false;
            if (joueur == 1)
            {
                for (int j = 6; j < 12; j++)
                {
                    if (jeu->plateau[j] > 0)
                        adversaire_nourri = true;
                }
            }
            else
            {
                for (int j = 0; j < 6; j++)
                {
                    if (jeu->plateau[j] > 0)
                        adversaire_nourri = true;
                }
            }

            // Annuler le mouvement pour restaurer l'état du plateau
            pos = i;
            for (int j = 0; j < graines; j++)
            {
                pos = (pos + 1) % 12;
                jeu->plateau[pos]--;
            }
            jeu->plateau[i] = graines;

            if (adversaire_nourri)
                return true;
        }
    }
    return false;
}

bool verifier_fin_partie(JeuAwale *jeu)
{
    // Vérifier si un joueur a atteint ou dépassé 24 points
    if (jeu->score_joueur1 >= 24 || jeu->score_joueur2 >= 24)
    {
        // Ramasser toutes les graines restantes
        for (int i = 0; i < TAILLE_PLATEAU; i++)
        {
            if (i < 6)
            {
                jeu->score_joueur1 += jeu->plateau[i];
            }
            else
            {
                jeu->score_joueur2 += jeu->plateau[i];
            }
            jeu->plateau[i] = 0;
        }
        return true;
    }

    bool joueur1_vide = true;
    bool joueur2_vide = true;

    for (int i = 0; i < 6; i++)
    {
        if (jeu->plateau[i] > 0)
            joueur1_vide = false;
    }
    for (int i = 6; i < 12; i++)
    {
        if (jeu->plateau[i] > 0)
            joueur2_vide = false;
    }

    if (joueur1_vide || joueur2_vide)
    {
        if (!nourrir_adversaire_possible(jeu, joueur1_vide ? 2 : 1))
        {
            // Ramasser toutes les graines restantes
            for (int i = 0; i < TAILLE_PLATEAU; i++)
            {
                if (jeu->plateau[i] > 0)
                {
                    if (i < 6)
                    {
                        jeu->score_joueur1 += jeu->plateau[i];
                    }
                    else
                    {
                        jeu->score_joueur2 += jeu->plateau[i];
                    }
                    jeu->plateau[i] = 0;
                }
            }
            return true;
        }
    }

    return false;
}