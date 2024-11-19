#include "commands.h"
#include <math.h>

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

void clean_invalid_parties(Client *clients, int actual)
{
    for (int i = partie_count - 1; i >= 0; i--)
    {
        bool challenger_present = false;
        bool challenged_present = false;
        PartieAwale *partie = &awale_parties[i];
        
        for (int j = 0; j < actual; j++)
        {
            if (strcmp(clients[j].name, partie->awale_challenge.challenger) == 0)
            {
                challenger_present = true;
                if (clients[j].partie_index == -1)
                {
                    remove_partie(i, clients);
                    continue;
                }
            }
            if (strcmp(clients[j].name, partie->awale_challenge.challenged) == 0)
            {
                challenged_present = true;
                if (clients[j].partie_index == -1)
                {
                    remove_partie(i, clients);
                    continue;
                }
            }
        }
        
        if (!challenger_present || !challenged_present)
        {
            remove_partie(i, clients);
        }
    }
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
    // Ensuite vérifier les défis en cours
    for (int i = 0; i < challenge_count; i++) {
        if (strcmp(awale_challenges[i].challenger, name) == 0 ||
            strcmp(awale_challenges[i].challenged, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Fonction auxiliaire pour ajouter un défi
void add_challenge(const char *challenger, const char *challenged, const char *message_rest)
{
    printf("debug message rest : %s\n", message_rest);
    if (challenge_count < MAX_CHALLENGES)
    {
        strncpy(awale_challenges[challenge_count].challenger, challenger, PSEUDO_MAX_LENGTH - 1);
        awale_challenges[challenge_count].challenger[PSEUDO_MAX_LENGTH - 1] = '\0'; // Ensure null termination
        strncpy(awale_challenges[challenge_count].challenged, challenged, PSEUDO_MAX_LENGTH - 1);
        awale_challenges[challenge_count].challenger[PSEUDO_MAX_LENGTH - 1] = '\0'; // Ensure null termination

        // form buffer yes/no:pseudo1,pseudo2,pseudo3
        char message_copy[strlen(message_rest) + 1];
        strcpy(message_copy, message_rest); // Make a copy of message_rest to avoid modifying the original string

        char *prive = strtok(message_copy, ":");
        if (prive == NULL)
        {
            awale_challenges[challenge_count].prive = false;
            awale_challenges[challenge_count].private_spec[0][0] = '\0';
            awale_challenges[challenge_count].private_spec_count = 0;
        }
        
        if (strcmp(prive, "yes") == 0)
        {
            awale_challenges[challenge_count].prive = true;
            // form pseudo1:pseudo2:pseudo3:
            char *private_spec = strtok(NULL, ":");
            while (private_spec != NULL)
            {
                printf("debug private spec : %s\n", private_spec);
                int count = awale_challenges[challenge_count].private_spec_count;
                strncpy(awale_challenges[challenge_count].private_spec[count], private_spec, PSEUDO_MAX_LENGTH - 1);
                awale_challenges[challenge_count].private_spec[count][PSEUDO_MAX_LENGTH - 1] = '\0'; // Ensure null termination
                awale_challenges[challenge_count].private_spec_count++;
                private_spec = strtok(NULL, ":");
            }

        }
        else
        {
            awale_challenges[challenge_count].prive = false;
            awale_challenges[challenge_count].private_spec[0][0] = '\0';
            awale_challenges[challenge_count].private_spec_count = 0;
        }
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

// In commands.c - Updated handle_quit_game function
void handle_quit_game(Client *clients, int actual, int client_index) {
    if (clients[client_index].partie_index != -1) {
        int partie_index = clients[client_index].partie_index;
        PartieAwale *partie = &awale_parties[partie_index];
        bool isSpectator = true;
        
        // Check if client is a player or spectator
        if (strcmp(partie->awale_challenge.challenger, clients[client_index].name) == 0 ||
            strcmp(partie->awale_challenge.challenged, clients[client_index].name) == 0) {
            isSpectator = false;
        }

        if (isSpectator) {
            // Handle spectator leaving
            for (int i = 0; i < partie->nbSpectators; i++) {
                if (strcmp(partie->Spectators[i].name, clients[client_index].name) == 0) {
                    // Remove spectator and shift remaining spectators
                    memmove(&partie->Spectators[i], &partie->Spectators[i + 1], 
                            (partie->nbSpectators - i - 1) * sizeof(Client));
                    partie->nbSpectators--;
                    break;
                }
            }
            // Reset spectator's partie_index
            clients[client_index].partie_index = -1;
            
        } else {
            // Existing player quit handling code...
            int challenge_index = find_challenge(clients[client_index].name);
            if (challenge_index != -1) {
                const char *challenger = awale_challenges[challenge_index].challenger;
                const char *challenged = awale_challenges[challenge_index].challenged;
                const char *other_player = strcmp(clients[client_index].name, challenger) == 0 ? challenged : challenger;
                
                for (int i = 0; i < actual; i++) {
                    if (strcmp(clients[i].name, other_player) == 0) {
                        char msg[BUF_SIZE];
                        snprintf(msg, BUF_SIZE, "\033[1;31mChallenge cancelled: %s has disconnected\033[0m\n",
                                clients[client_index].name);
                        write_client(clients[i].sock, msg);
                        break;
                    }
                }
                remove_challenge(challenge_index);
            }

            // Notify all players and spectators about game end
            char msg[BUF_SIZE];
            snprintf(msg, BUF_SIZE, "Game over! %s has left the game\n", clients[client_index].name);
            
            for (int i = 0; i < actual; i++) {
                if (clients[i].partie_index == partie_index) {
                    write_client(clients[i].sock, msg);
                    clients[i].partie_index = -1;
                }
            }
            remove_partie(partie_index, clients);
        }
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
        PartieAwale nouvelle_partie = {0};
        JeuAwale jeu;
        initialiser_plateau(&jeu);
        nouvelle_partie.jeu = jeu;
        srand(time(NULL));
        nouvelle_partie.tour = rand() % 2 + 1;
        nouvelle_partie.awale_challenge = awale_challenges[challenge_index];
        nouvelle_partie.cout[0] = '\0';
        nouvelle_partie.cout_index = 0;
        nouvelle_partie.in_save = false;
        nouvelle_partie.nbSpectators = 0;

        // Mettre l'id dans les 2 clients
        for (int i = 0; i < actual; i++)
        {
            if (strcmp(clients[i].name, challenger) == 0 ||
                strcmp(clients[i].name, challenged) == 0)
            {
                clients[i].partie_index = partie_count;
            }
        }

        // Préparer le message pour le plateau initial
        char plateau_initial[BUF_SIZE] = {0};
        int offset = 0;
        offset += snprintf(plateau_initial, BUF_SIZE, "AWALE:");
        
        for (int i = 0; i < TAILLE_PLATEAU; i++)
        {
            offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, "%d:", nouvelle_partie.jeu.plateau[i]);
        }

        // Ajouter le pseudo du joueur qui commence et son numéro
        if (nouvelle_partie.tour == 1)
        {
            offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, "%s:%d",
                             challenger, nouvelle_partie.tour);
            if (nouvelle_partie.cout_index <= BUF_SAVE_SIZE - 1)
            {
                snprintf(nouvelle_partie.cout, BUF_SAVE_SIZE, "%s:%s:%d",
                        challenger, challenged, nouvelle_partie.tour);
                nouvelle_partie.cout_index = strlen(nouvelle_partie.cout);
            }
        }
        else
        {
            offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, "%s:%d",
                             challenged, nouvelle_partie.tour);
            if (nouvelle_partie.cout_index <= BUF_SAVE_SIZE - 1)
            {
                snprintf(nouvelle_partie.cout, BUF_SAVE_SIZE, "%s:%s:%d",
                        challenger, challenged, nouvelle_partie.tour);
                nouvelle_partie.cout_index = strlen(nouvelle_partie.cout);
            }
        }

        // Sauvegarder la partie avant de supprimer le challenge
        awale_parties[partie_count] = nouvelle_partie;
        partie_count++;

        // Ajouter les scores des joueurs
        offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, ":%d:%d",
                         nouvelle_partie.jeu.score_joueur1, nouvelle_partie.jeu.score_joueur2);

        // Envoyer les messages aux joueurs
        for (int i = 0; i < actual; i++)
        {
            if (strcmp(clients[i].name, challenger) == 0 ||
                strcmp(clients[i].name, challenged) == 0)
            {
                write_client(clients[i].sock, start_msg);
#ifdef WIN32
                Sleep(1000);
#else
                sleep(1);
#endif
                write_client(clients[i].sock, plateau_initial);
            }
        }

        // Supprimer le challenge après que tout soit configuré
        //remove_challenge(challenge_index);
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
        remove_challenge(challenge_index);
    }
}

void handle_awale_challenge(Client *clients, int actual, int client_index, char *buffer)
{
    printf("Partie count : %d\n", partie_count);
    printf("Challenge count : %d\n", challenge_count);
    // Vérifier si le challenger a déjà un défi en cours
    if (find_challenge(clients[client_index].name) != -1)
    {
        char response[BUF_SIZE];
        snprintf(response, BUF_SIZE, "You already have a pending challenge.\n");
        write_client(clients[client_index].sock, response);
        return;
    }

    char * target_pseudo = strtok(buffer, ":");
    char * message_rest = strtok(NULL, "");
    printf("buffer restant : %s\n", message_rest);

    if (target_pseudo == NULL || message_rest == NULL)
    {
        char response[BUF_SIZE];
        snprintf(response, BUF_SIZE, "Invalid challenge format.\n");
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
            add_challenge(clients[client_index].name, target_pseudo, message_rest);
            printf("after add challenge value of challenge count : %d\n", challenge_count);
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

        // Mettre a jour cout
        if (partie->cout_index <= BUF_SAVE_SIZE - 1)
        {
            partie->cout[partie->cout_index] = ':';
            partie->cout_index++;
        }
        if (partie->cout_index <= BUF_SAVE_SIZE - 2)
        {
            // Ajouter le coup au cout (attention 10, 11 sonbt des chiffres à 2 chiffres)
            if (case_depart < 10)
            {
                partie->cout[partie->cout_index] = case_depart + '0';
                partie->cout_index++;
            }
            else
            {
                partie->cout[partie->cout_index] = '1';
                partie->cout_index++;
                partie->cout[partie->cout_index] = (case_depart - 10) + '0';
                partie->cout_index++;
            }
        }
        printf("Debug - Cout : %s\n", partie->cout); // Debug


        // Mettre à jour le tour
        partie->tour = (joueur == 1) ? 2 : 1;

        // Vérifier si la partie est terminée
        if (verifier_fin_partie(jeu))
        {
            char end_msg[BUF_SIZE];
            if (jeu->score_joueur1 == jeu->score_joueur2)
            {
                snprintf(end_msg, BUF_SIZE, "Game over! It's a draw! Final score: %d-%d\n",
                        jeu->score_joueur1, jeu->score_joueur2);
            }
            else
            {
                snprintf(end_msg, BUF_SIZE, "Game over! %s wins with a score of %d-%d!\n",
                        (jeu->score_joueur1 > jeu->score_joueur2) ? 
                        partie->awale_challenge.challenger : partie->awale_challenge.challenged,
                        jeu->score_joueur1, jeu->score_joueur2);
            }

            // Envoyer le message de fin de partie
            for (int i = 0; i < actual; i++)
            {
                if (strcmp(clients[i].name, partie->awale_challenge.challenger) == 0 ||
                    strcmp(clients[i].name, partie->awale_challenge.challenged) == 0)
                {
                    //clients[i].partie_index = -1;
                    write_client(clients[i].sock, end_msg);
                }
            }

            // ENvoyer le message de fin de partie aux spectateurs avec [Spectator] devant
            char end_msg_spectator[BUF_SIZE];
            snprintf(end_msg_spectator, BUF_SIZE, "[Spectator] %s\n", end_msg);
            for (int i = 0; i < partie->nbSpectators; i++)
            {
                write_client(partie->Spectators[i].sock, end_msg_spectator);
            }

            Client * challenger = findClientByPseudo(clients, actual, partie->awale_challenge.challenger);
            Client * challenged = findClientByPseudo(clients, actual, partie->awale_challenge.challenged);

            int facteur = 32;

            int classement_challenger = challenger->point;
            int classement_challenged = challenged->point;

            // Maj du score des joueurs
            float part_of_calcul_challenged = (float) (classement_challenger - classement_challenged) / 400;
            float part_of_calcul_challenger = (float) (classement_challenged - classement_challenger) / 400;
            float p_vict_challenger = 1/(1+pow(10, part_of_calcul_challenger));
            float p_vict_challenged = 1/(1+pow(10, part_of_calcul_challenged));
            printf("ratio challenger : %f\n", p_vict_challenger);
            printf("ratio challenged : %f\n", p_vict_challenged);

            // Update des points
            if (jeu->score_joueur1 == jeu->score_joueur2)
            {
                challenger->point += facteur*(0.5-p_vict_challenger);
                challenged->point += facteur*(0.5-p_vict_challenged);
            }
            else if (jeu->score_joueur1 > jeu->score_joueur2)
            {
                challenger->point += facteur*(1-p_vict_challenger);
                challenged->point += facteur*(0-p_vict_challenged);
            }
            else
            {
                challenger->point += facteur*(0-p_vict_challenger);
                challenged->point += facteur*(1-p_vict_challenged);
            }

            printf("classement challenger : %d\n", challenger->point);
            printf("classement challenged : %d\n", challenged->point);

            // In_save
            partie->in_save = true;

            return;
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

            for (int i = 0; i < partie->nbSpectators; i++)
            {
                write_client(partie->Spectators[i].sock, plateau_updated);
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

void handle_save(Client *clients, int actual, int client_index, const char *buffer)
{
    char response[BUF_SIZE];
    char *save = strdup(buffer);
    printf("save: %s\n", save);
    printf("buffer: %s\n", buffer);

    // Trouver la partie
    int partie_index = clients[client_index].partie_index;
    if (partie_index == -1)
    {
        free(save);
        return;
    }

    PartieAwale *partie = &awale_parties[partie_index];
    
    // Trouver les deux joueurs
    Client *challenger = findClientByPseudo(clients, actual, partie->awale_challenge.challenger);
    Client *challenged = findClientByPseudo(clients, actual, partie->awale_challenge.challenged);
    
    if (!challenger || !challenged)
    {
        free(save);
        return;
    }

    // Enregistrer la réponse et marquer que ce joueur a répondu
    if (strcmp(save, "yes") == 0)
    {
        if (partie->cout_index <= BUF_SAVE_SIZE - 1)
        {
            snprintf(response, BUF_SAVE_SIZE, "%s\n", partie->cout);
        }
        else
        {
            snprintf(response, BUF_SAVE_SIZE, "%d\n", -1);
        }
        write_client(clients[client_index].sock, response);
    }
    else
    {
        snprintf(response, BUF_SAVE_SIZE, "%d\n", -1);
        write_client(clients[client_index].sock, response);
    }

    // Marquer que ce joueur a répondu
    clients[client_index].partie_index = -1;

    free(save);

    // Si les deux joueurs ont répondu (-1), on peut nettoyer la partie
    if (challenger->partie_index == -1 && challenged->partie_index == -1)
    {
        int partie_challenge_index = find_challenge(partie->awale_challenge.challenger);
        if (partie_challenge_index != -1)
        {
            remove_challenge(partie_challenge_index);
        }
        remove_partie(partie_index, clients);
    }
}

void handle_private_message(Client *clients, int actual, int sender_index, const char *buffer)
{
    char target_pseudo[BUF_SIZE];
    char message[BUF_SIZE];
    const char *msg_start = strchr(buffer + 3, ':');

    printf("buffer: %s\n", buffer);
    printf("msg_start: %s\n", msg_start);

    if (msg_start == NULL)
        return;

    int pseudo_len = msg_start - (buffer + 3);
    strncpy(target_pseudo, buffer + 3, pseudo_len);
    target_pseudo[pseudo_len] = '\0';

    printf("target_pseudo: %s\n", target_pseudo);

    strncpy(message, msg_start + 1, BUF_SIZE - 1);

    for (int i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, target_pseudo) == 0)
        {
            char formatted_msg[BUF_SIZE];
            snprintf(formatted_msg, BUF_SIZE, "[Private from %s]: %s\n\n",
                     clients[sender_index].name, message);
            write_client(clients[i].sock, formatted_msg);

            snprintf(formatted_msg, BUF_SIZE, "[Private to %s]: %s\n\n",
                     target_pseudo, message);
            write_client(clients[sender_index].sock, formatted_msg);
            return;
        }
    }

    char error_msg[BUF_SIZE];
    snprintf(error_msg, BUF_SIZE, "User %s not found.\n", target_pseudo);
    write_client(clients[sender_index].sock, error_msg);
    //free(msg_start);
}

void list_connected_clients(Client *clients, int actual, int requester_index)
{
    char list[BUF_SIZE * MAX_CLIENTS] = "Connected users:\n";

    for (int i = 0; i < actual; i++)
    {
        if (clients[i].sock != INVALID_SOCKET)
        {
            // name + rank
            char user_entry[BUF_SIZE];
            snprintf(user_entry, BUF_SIZE, "- %s : %d points\n", clients[i].name, clients[i].point);
            strcat(list, user_entry);
        }
    }

    write_client(clients[requester_index].sock, list);
}

void handle_spec(Client *clients, int actual, int client_index, const char *buffer)
{
    printf("buffer: %s\n", buffer); //pseudo

    // Trouver la partie
    Client * player = findClientByPseudo(clients, actual, buffer);
    if (player == NULL)
    {
        write_client(clients[client_index].sock, "FAIL:This player doesnt exist\n");
        return;
    }

    if (player->partie_index == -1)
    {
        write_client(clients[client_index].sock, "FAIL:This player is not in a game\n");
        return;
    }

    PartieAwale * partie = &awale_parties[player->partie_index];
    if (partie == NULL || partie->nbSpectators >= MAX_CLIENTS || partie->in_save)
    {
        write_client(clients[client_index].sock, "FAIL:Cannot spectate this game (doesnt exist, not ready or forbidden)\n");
        return;
    }

    AwaleChallenge * challenge = &partie->awale_challenge;
    bool is_private = challenge->prive;
    bool is_in_private_spec = false;

    if (is_private)
    {
        for (int i = 0; i < challenge->private_spec_count; i++)
        {
            printf("debug private spec: %s\n", challenge->private_spec[i]);
            if (strcmp(clients[client_index].name, challenge->private_spec[i]) == 0)
            {
                is_in_private_spec = true;
                break;
            }
        }
    }

    printf("is_private: %d\n", is_private);
    printf("is_in_private_spec: %d\n", is_in_private_spec);

    if (is_private && !is_in_private_spec)
    {
        write_client(clients[client_index].sock, "FAIL:This game is private and not for you\n");
        return;
    }


    // Ajouter le client à la liste des spectateurs
    partie->Spectators[partie->nbSpectators] = clients[client_index];
    partie->nbSpectators++;
    
    // Le client est in_game
    clients[client_index].partie_index = player->partie_index;
    
    write_client(clients[client_index].sock, "You are now spectating this game\n");
    sleep(1);
    

    // Envoyer le plateau actuel
    // TODO: Envoyer le plateau actuel
    char plateau_updated[BUF_SIZE] = {0};
    int offset = 0;
    offset += snprintf(plateau_updated, BUF_SIZE, "AWALE:");
    for (int i = 0; i < TAILLE_PLATEAU; i++)
    {
        offset += snprintf(plateau_updated + offset, BUF_SIZE - offset, "%d:", partie->jeu.plateau[i]);
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
                       partie->jeu.score_joueur1, partie->jeu.score_joueur2);
    
    write_client(clients[client_index].sock, plateau_updated);
}

void handle_awale_list(Client *clients, int actual, int client_index)
{
    //clean_invalid_parties(clients, actual);
    char list[BUF_SIZE * MAX_PARTIES] = "Games in progress:\n";

    printf("partie_count cote awale list: %d\n", partie_count);

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

/*void addSpectator(PartieAwale *partieAwale, Client newSpectator)
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
}*/

/*void initSpectators(Client *clients, int actual, PartieAwale *partieAwale) // remember to free this memory
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
}*/


/*void allowAll(Client *clients, int actual, PartieAwale *partieAwale)
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

        if (player1->sock == partieAwale->spectators[i].sock || player2->sock==partieAwale->spectators[i].sock )
        {
            continue;
        }
        memset(message, 0, sizeof(message));
        strncpy(message, buffer, BUF_SIZE - 1);
        message[BUF_SIZE - 1] = '\0';
        // Send the message to the spectators
        write_client(partieAwale->spectators[i].sock, message);
    }
}*/