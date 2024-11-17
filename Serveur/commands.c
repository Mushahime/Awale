#include "commands.h"

// Modifications dans la fonction check_pseudo dans server2.c
int check_pseudo(Client *clients, int actual, const char *pseudo)
{
    // Vérification de la longueur
    size_t len = strlen(pseudo);
    if (len < PSEUDO_MIN_LENGTH || len >= PSEUDO_MAX_LENGTH)
    {
        return 0;
    }

    // Vérification des caractères valides (lettres, chiffres, underscore)
    for (size_t i = 0; i < len; i++)
    {
        if (!isalnum(pseudo[i]) && pseudo[i] != '_')
        {
            return 0;
        }
    }

    // Vérification de l'unicité
    for (int i = 0; i < actual; i++)
    {
        if (strcasecmp(clients[i].name, pseudo) == 0)
        { // Case-insensitive comparison
            return 0;
        }
    }

    return 1;
}

void handle_bio_command(Client *clients, int actual, int client_index, const char *buffer)
{
    char response[BUF_SIZE];

    if (strncmp(buffer, "setbio:", 7) == 0)
    {
        strncpy(clients[client_index].bio, buffer + 7, MAX_BIO_LENGTH - 1);
        clients[client_index].has_bio = 1;
        strcpy(response, "Bio updated successfully!\n");
        write_client(clients[client_index].sock, response);
    }
    else if (strncmp(buffer, "getbio:", 7) == 0)
    {
        char target_pseudo[BUF_SIZE];
        strncpy(target_pseudo, buffer + 7, BUF_SIZE - 1);

        int found = 0;
        for (int i = 0; i < actual; i++)
        {
            if (strcmp(clients[i].name, target_pseudo) == 0)
            {
                if (clients[i].has_bio)
                {
                    snprintf(response, BUF_SIZE, "Bio of %s:\n%s\n", target_pseudo, clients[i].bio);
                }
                else
                {
                    snprintf(response, BUF_SIZE, "%s hasn't set their bio yet.\n", target_pseudo);
                }
                write_client(clients[client_index].sock, response);
                found = 1;
                break;
            }
        }

        if (!found)
        {
            snprintf(response, BUF_SIZE, "User %s not found.\n", target_pseudo);
            write_client(clients[client_index].sock, response);
        }
    }
}

int find_challenge(const char *name)
{
    for (int i = 0; i < challenge_count; i++)
    {
        // On vérifie si le nom correspond soit au challenger soit au challenged
        if (strcmp(awale_challenges[i].challenger, name) == 0 ||
            strcmp(awale_challenges[i].challenged, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

// Fonction auxiliaire pour ajouter un défi
void add_challenge(const char *challenger, const char *challenged)
{
    if (challenge_count < MAX_CHALLENGES)
    {
        strncpy(awale_challenges[challenge_count].challenger, challenger, PSEUDO_MAX_LENGTH - 1);
        strncpy(awale_challenges[challenge_count].challenged, challenged, PSEUDO_MAX_LENGTH - 1);
        challenge_count++;
    }
}

// Fonction auxiliaire pour supprimer un défi
void remove_challenge(int index)
{
    if (index >= 0 && index < challenge_count)
    {
        memmove(&awale_challenges[index], &awale_challenges[index + 1],
                (challenge_count - index - 1) * sizeof(AwaleChallenge));
        challenge_count--;
    }
}

void handle_awale_response(Client *clients, int actual, int client_index, const char *response)
{
    int challenge_index = find_challenge(clients[client_index].name);
    if (challenge_index == -1)
    {
        return;
    }

    char start_msg[BUF_SIZE];
    char *challenger = awale_challenges[challenge_index].challenger;
    char *challenged = awale_challenges[challenge_index].challenged;

    if (strcmp(response, "yes") == 0)
    {
        // Vérifier s'il y a de la place pour une nouvelle partie
        if (partie_count >= MAX_PARTIES)
        {
            char error_msg[BUF_SIZE];
            snprintf(error_msg, BUF_SIZE, "Server cannot handle more games at the moment.\n");
            write_client(clients[client_index].sock, error_msg);
            remove_challenge(challenge_index);
            return;
        }

        snprintf(start_msg, BUF_SIZE, "[Challenge] Game started between %s and %s\n",
                 challenger, challenged);

        // Créer et initialiser la partie
        PartieAwale nouvelle_partie;
        nouvelle_partie.nbSpectators = 2;

        JeuAwale jeu;
        initialiser_plateau(&jeu);
        nouvelle_partie.jeu = jeu;
        srand(time(NULL));
        nouvelle_partie.tour = rand() % 2 + 1;
        printf("Debug - Tour initial : %d\n", nouvelle_partie.tour); // Debug
        nouvelle_partie.awale_challenge = awale_challenges[challenge_index];

        // Sauvegarder la partie dans le tableau
        awale_parties[partie_count] = nouvelle_partie;

        // Mettre l'id dans les 2 clients
        for (int i = 0; i < actual; i++)
        {
            if (strcmp(clients[i].name, challenger) == 0)
            {
                clients[i].partie_index = partie_count;
            }
            if (strcmp(clients[i].name, challenged) == 0)
            {
                clients[i].partie_index = partie_count;
            }
        }

        // Préparer le message pour le plateau initial
        char plateau_initial[BUF_SIZE] = {0}; // Initialisation à zéro
        int offset = 0;
        offset += snprintf(plateau_initial, BUF_SIZE, "AWALE:");
        // Ajouter les valeurs du plateau
        for (int i = 0; i < TAILLE_PLATEAU; i++)
        {
            offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, "%d:", nouvelle_partie.jeu.plateau[i]);
        }

        // Ajouter le pseudo du joueur qui commence et son numéro
        if (nouvelle_partie.tour == 1)
        {
            offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, "%s:%d",
                               challenger, nouvelle_partie.tour);
        }
        else
        {
            offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, "%s:%d",
                               challenged, nouvelle_partie.tour);
        }

        // Ajouter les scores des joueurs
        offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, ":%d:%d",
                           nouvelle_partie.jeu.score_joueur1, nouvelle_partie.jeu.score_joueur2);

        printf("Debug - Message construit : %s\n", plateau_initial); // Debug

        // Mettre à jour les clients avec l'index de la partie
        for (int i = 0; i < actual; i++)
        {
            if (strcmp(clients[i].name, challenger) == 0 ||
                strcmp(clients[i].name, challenged) == 0)
            {
                write_client(clients[i].sock, start_msg);
#ifdef WIN32
                Sleep(2000); // Windows
#else
                sleep(2); // Unix
#endif
                write_client(clients[i].sock, plateau_initial);
            }
        }

        partie_count++;
    }
    else
    {
        // Jeu refusé
        snprintf(start_msg, BUF_SIZE, "[Challenge] Game declined by %s\n", challenged);
        for (int i = 0; i < actual; i++)
        {
            if (strcmp(clients[i].name, challenger) == 0)
            {
                write_client(clients[i].sock, start_msg);
                break;
            }
        }
    }

    remove_challenge(challenge_index);
}

void handle_awale_challenge(Client *clients, int actual, int client_index, const char *target_pseudo)
{
    // Vérifier si le challenger a déjà un défi en cours
    if (find_challenge(clients[client_index].name) != -1)
    {
        char response[BUF_SIZE];
        snprintf(response, BUF_SIZE, "You already have a pending challenge.\n");
        write_client(clients[client_index].sock, response);
        return;
    }

    // Chercher le joueur cible
    int found = 0;
    for (int i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, target_pseudo) == 0)
        {
            // Vérifier si la cible a déjà un défi en cours
            if (find_challenge(target_pseudo) != -1)
            {
                char response[BUF_SIZE];
                snprintf(response, BUF_SIZE, "Player %s is already in a challenge.\n", target_pseudo);
                write_client(clients[client_index].sock, response);
                return;
            }

            // Ajouter le défi et envoyer la demande
            add_challenge(clients[client_index].name, target_pseudo);
            char challenge_msg[BUF_SIZE];
            snprintf(challenge_msg, BUF_SIZE, "Awale fight request from %s\n",
                     clients[client_index].name);
            write_client(clients[i].sock, challenge_msg);
            found = 1;
            break;
        }
    }

    if (!found)
    {
        char response[BUF_SIZE];
        snprintf(response, BUF_SIZE, "User %s not found.\n", target_pseudo);
        write_client(clients[client_index].sock, response);
    }
}

void handle_awale_move(Client *clients, int actual, int client_index, const char *move)
{
    int partie_index = clients[client_index].partie_index;
    if (partie_index == -1)
    {
        return;
    }

    PartieAwale *partie = &awale_parties[partie_index];
    JeuAwale *jeu = &partie->jeu;

    int case_depart = atoi(move);
    int joueur = partie->tour;

    // Vérification case vide (ça aurait pu être traité après jouer coup)
    if (jeu->plateau[case_depart] == 0)
    {
        char error_msg[BUF_SIZE];
        // envoyer le numéro du joueur qui a joué le coup et le message d'erreur
        snprintf(error_msg, BUF_SIZE, "ERROR:%s:%d\n", INVALID_MOVE_EMPTY, joueur);
        printf("error_msg: %s\n", error_msg);
        write_client(clients[client_index].sock, error_msg);
        return;
    }

    bool coup_valide = jouer_coup(jeu, joueur, case_depart);

    if (coup_valide)
    {
        // Mettre à jour le tour
        partie->tour = (joueur == 1) ? 2 : 1;

        // Vérifier si la partie est terminée
        if (verifier_fin_partie(jeu))
        {
            char end_msg[BUF_SIZE];
            snprintf(end_msg, BUF_SIZE, "Game over! %s wins!\n",
                     (jeu->score_joueur1 > jeu->score_joueur2) ? partie->awale_challenge.challenger : partie->awale_challenge.challenged);

            // Réinitialiser les indices de partie pour les deux joueurs
            for (int i = 0; i < actual; i++)
            {
                if (strcmp(clients[i].name, partie->awale_challenge.challenger) == 0 ||
                    strcmp(clients[i].name, partie->awale_challenge.challenged) == 0)
                {
                    clients[i].partie_index = -1;
                    write_client(clients[i].sock, end_msg);
                }
            }
            // Libérez challenge
            int partie_challenge_index = find_challenge(partie->awale_challenge.challenger);
            if (partie_challenge_index != -1)
            {
                remove_challenge(partie_challenge_index);
            }
            // Libérer la partie
            remove_partie(partie_index);
        }
        else
        {
            // Envoyer le plateau mis à jour
            char plateau_updated[BUF_SIZE] = {0};
            int offset = 0;
            offset += snprintf(plateau_updated, BUF_SIZE, "AWALE:");
            for (int i = 0; i < TAILLE_PLATEAU; i++)
            {
                offset += snprintf(plateau_updated + offset, BUF_SIZE - offset, "%d:", jeu->plateau[i]);
            }
            if (partie->tour == 1)
            {
                offset += snprintf(plateau_updated + offset, BUF_SIZE - offset, "%s:%d",
                                   partie->awale_challenge.challenger, partie->tour);
            }
            else
            {
                offset += snprintf(plateau_updated + offset, BUF_SIZE - offset, "%s:%d",
                                   partie->awale_challenge.challenged, partie->tour);
            }
            offset += snprintf(plateau_updated + offset, BUF_SIZE - offset, ":%d:%d",
                               jeu->score_joueur1, jeu->score_joueur2);

            for (int i = 0; i < actual; i++)
            {
                if (strcmp(clients[i].name, partie->awale_challenge.challenger) == 0 ||
                    strcmp(clients[i].name, partie->awale_challenge.challenged) == 0)
                {
                    write_client(clients[i].sock, plateau_updated);
                }
            }
        }
    }
    else
    {
        // Coup invalide pour famine
        char error_msg[BUF_SIZE];
        // envoyer le numéro du joueur qui a joué le coup et le message d'erreur
        snprintf(error_msg, BUF_SIZE, "ERROR:%s:%d\n", INVALID_MOVE_FAMINE, joueur);
        write_client(clients[client_index].sock, error_msg);
    }
}

void handle_private_message(Client *clients, int actual, int sender_index, const char *buffer)
{
    char target_pseudo[BUF_SIZE];
    char message[BUF_SIZE];
    const char *msg_start = strchr(buffer + 3, ':');

    if (msg_start == NULL)
        return;

    int pseudo_len = msg_start - (buffer + 3);
    strncpy(target_pseudo, buffer + 3, pseudo_len);
    target_pseudo[pseudo_len] = '\0';

    strncpy(message, msg_start + 1, BUF_SIZE - 1);

    for (int i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, target_pseudo) == 0)
        {
            char formatted_msg[BUF_SIZE];
            snprintf(formatted_msg, BUF_SIZE, "[Private from %s]: %s\n",
                     clients[sender_index].name, message);
            write_client(clients[i].sock, formatted_msg);

            snprintf(formatted_msg, BUF_SIZE, "[Private to %s]: %s\n",
                     target_pseudo, message);
            write_client(clients[sender_index].sock, formatted_msg);
            return;
        }
    }

    char error_msg[BUF_SIZE];
    snprintf(error_msg, BUF_SIZE, "User %s not found.\n", target_pseudo);
    write_client(clients[sender_index].sock, error_msg);
}

void list_connected_clients(Client *clients, int actual, int requester_index)
{
    char list[BUF_SIZE * MAX_CLIENTS] = "Connected users:\n";

    for (int i = 0; i < actual; i++)
    {
        if (clients[i].sock != INVALID_SOCKET)
        {
            char user_entry[BUF_SIZE];
            snprintf(user_entry, BUF_SIZE, "- %s\n", clients[i].name);
            strcat(list, user_entry);
        }
    }

    write_client(clients[requester_index].sock, list);
}

void handle_awale_list(Client *clients, int actual, int client_index)
{
    char list[BUF_SIZE * MAX_PARTIES] = "Games in progress:\n";

    for (int i = 0; i < partie_count; i++)
    {
        char game_entry[BUF_SIZE];
        snprintf(game_entry, BUF_SIZE, "- %s vs %s\n",
                 awale_parties[i].awale_challenge.challenger,
                 awale_parties[i].awale_challenge.challenged);
        strcat(list, game_entry);
    }

    if (partie_count == 0)
    {
        strcat(list, "0 game in progress\n");
    }

    write_client(clients[client_index].sock, list);
}

void addSpectator(PartieAwale *partieAwale, Client newSpectator)
{
    int current_size = partieAwale->nbSpectators + 1;
    int *temp = realloc(partieAwale->spectators, current_size * sizeof(int)); // Reallocate memory
    if (temp == NULL)
    {
        perror("Failed to reallocate memory");
        free(partieAwale->spectators); // Free the old memory to avoid memory leaks
        exit(EXIT_FAILURE);
    }
    partieAwale->spectators = temp;
    partieAwale->spectators[current_size - 1] = newSpectator;
    partieAwale->nbSpectators += 1;
}

void initSpectators(Client *clients, int actual, PartieAwale *partieAwale) // remember to free this memory
{
    int initialSize = partieAwale->nbSpectators;
    partieAwale = malloc(initialSize * sizeof(Client));
    Client *player1 = findClientByPseudo(clients, actual, partieAwale->awale_challenge.challenged);
    if (player1 != NULL)
    {
        partieAwale->spectators[0] = *player1;
    }
    Client *player2 = findClientByPseudo(clients, actual, partieAwale->awale_challenge.challenger);
    if (player2 != NULL)
    {
        partieAwale->spectators[1] = *player2;
    }
}


void allowAll(Client *clients, int actual, PartieAwale *partieAwale)
{
    for (int i = 0; i < actual; i++)
    {
        partieAwale->spectators[i] = clients[i];
    }
    partieAwale->nbSpectators = actual;
}

void clearSpectators(PartieAwale *PartieAwale)
{
    free(PartieAwale->spectators);
}

void stream_move(SOCKET sock, const char *buffer, PartieAwale *partieAwale)
{
    char message[BUF_SIZE];
    Client *player1 = findClientByPseudo(partieAwale->spectators, partieAwale->nbSpectators, partieAwale->awale_challenge.challenged);
    Client *player2 = findClientByPseudo(partieAwale->spectators, partieAwale->nbSpectators, partieAwale->awale_challenge.challenger);
    for (int i = 0; i < partieAwale->nbSpectators; i++)
    {

        if (player1->sock == partieAwale->spectators[i].sock || player2-sock==partieAwale->spectators[i].sock )
        {
            continue;
        }
        memset(message, 0, sizeof(message));
        strncpy(message, buffer, BUF_SIZE - 1);
        message[BUF_SIZE - 1] = '\0';
        // Send the message to the spectators
        write_client(partieAwale->spectators[i].sock, message);
    }
}