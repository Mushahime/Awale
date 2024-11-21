#include "commands.h"
#include <math.h>

// Function to check if a pseudo is valid
int check_pseudo(Client *clients, int actual, const char *pseudo)
{
    // length check
    size_t len = strlen(pseudo);
    if (len < PSEUDO_MIN_LENGTH || len >= PSEUDO_MAX_LENGTH)
    {
        return 0;
    }

    // character check
    for (size_t i = 0; i < len; i++)
    {
        if (!isalnum(pseudo[i]) && pseudo[i] != '_')
        {
            return 0;
        }
    }

    // pseudo check
    for (int i = 0; i < actual; i++)
    {
        if (strcasecmp(clients[i].name, pseudo) == 0)
        { // Case-insensitive comparison
            return 0;
        }
    }

    return 1;
}

// Function to delete invalid games
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

// Function to handle bio commands (setbio and getbio)
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

// Function to find a challenge in the list
int find_challenge(const char *name)
{
    // check if the player is a challenger or challenged in a challenge
    for (int i = 0; i < challenge_count; i++) {
        if (strcmp(awale_challenges[i].challenger, name) == 0 ||
            strcmp(awale_challenges[i].challenged, name) == 0) {
            return i;
        }
    }

    // check if the player is a spectator in a game
    for (int i = 0; i < partie_count; i++) {
        PartieAwale *partie = &awale_parties[i];
        for (int j = 0; j < partie->nbSpectators; j++) {
            if (strcmp(partie->Spectators[j].name, name) == 0) {
                return i;
            }
        }
    }
    return -1;
}

// Function to add a challenge to the list
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

// Function to remove a challenge from the list
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
            // Handle spectator leaving (unchanged)
            for (int i = 0; i < partie->nbSpectators; i++) {
                if (strcmp(partie->Spectators[i].name, clients[client_index].name) == 0) {
                    memmove(&partie->Spectators[i], &partie->Spectators[i + 1], 
                            (partie->nbSpectators - i - 1) * sizeof(Client));
                    partie->nbSpectators--;
                    break;
                }
            }
            clients[client_index].partie_index = -1;
            
        } else {
            // Handle player leaving - now with Elo update
            const char *challenger = partie->awale_challenge.challenger;
            const char *challenged = partie->awale_challenge.challenged;
            const char *quitter = clients[client_index].name;
            const char *winner = strcmp(quitter, challenger) == 0 ? challenged : challenger;

            // Find the winner client
            Client *winner_client = NULL;
            Client *quitter_client = &clients[client_index];

            for (int i = 0; i < actual; i++) {
                if (strcmp(clients[i].name, winner) == 0) {
                    winner_client = &clients[i];
                    break;
                }
            }

            if (winner_client != NULL) {
                // Update Elo ratings - quitter loses
                update_elo_ratings(winner_client, quitter_client, false);

                char msg[BUF_SIZE];
                snprintf(msg, BUF_SIZE, "\nGame over! %s has left the game. %s wins by forfeit!\n", 
                        quitter, winner);
                
                // Notify all players and spectators
                for (int i = 0; i < actual; i++) {
                    if (clients[i].partie_index == partie_index) {
                        write_client(clients[i].sock, msg);
                        clients[i].partie_index = -1;
                    }
                }
            }

            int challenge_index = find_challenge(clients[client_index].name);
            if (challenge_index != -1) {
                remove_challenge(challenge_index);
            }
            remove_partie(partie_index, clients);
        }
    }
}

//function to react after the response of a challenged player
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
        // Check if the server can handle more games
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

        // Create a new game
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

        // Put the players in the game
        for (int i = 0; i < actual; i++)
        {
            if (strcmp(clients[i].name, challenger) == 0 ||
                strcmp(clients[i].name, challenged) == 0)
            {
                clients[i].partie_index = partie_count;
            }
        }

        // Prepared the initial board state
        char plateau_initial[BUF_SIZE] = {0};
        int offset = 0;
        offset += snprintf(plateau_initial, BUF_SIZE, "AWALE:");
        
        for (int i = 0; i < TAILLE_PLATEAU; i++)
        {
            offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, "%d:", nouvelle_partie.jeu.plateau[i]);
        }

        // Add the player who starts and their number
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

        // Save the game
        awale_parties[partie_count] = nouvelle_partie;
        partie_count++;

        // Add the scores
        offset += snprintf(plateau_initial + offset, BUF_SIZE - offset, ":%d:%d",
                         nouvelle_partie.jeu.score_joueur1, nouvelle_partie.jeu.score_joueur2);

        // Send the initial board state to the players
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

    }
    else
    {
        // Game declined
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

// Function after a challenge is asked by a player
void handle_awale_challenge(Client *clients, int actual, int client_index, char *buffer)
{
    printf("Partie count : %d\n", partie_count);
    printf("Challenge count : %d\n", challenge_count);
    // Check if the player already has a pending challenge
    if (find_challenge(clients[client_index].name) != -1)
    {
        char response[BUF_SIZE];
        snprintf(response, BUF_SIZE, "You already have a pending challenge.\n");
        sleep(0.5); // to avoid mix of messages
        write_client(clients[client_index].sock, response);
        return;
    }

    char * target_pseudo = strtok(buffer, ":");
    char * message_rest = strtok(NULL, "");

    if (target_pseudo == NULL || message_rest == NULL)
    {
        char response[BUF_SIZE];
        snprintf(response, BUF_SIZE, "Invalid challenge format.\n");
        write_client(clients[client_index].sock, response);
        return;
    }

    // Find the target player
    int found = 0;
    for (int i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, target_pseudo) == 0)
        {
            // Check if the target player already has a pending challenge
            if (find_challenge(target_pseudo) != -1)
            {
                char response[BUF_SIZE];
                snprintf(response, BUF_SIZE, "Player %s is already in a challenge.\n", target_pseudo);
                write_client(clients[client_index].sock, response);
                return;
            }

            // Add the challenge and send the request
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

// Function to handle a move in an Awale game
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

    // Empty case check
    if (jeu->plateau[case_depart] == 0)
    {
        char error_msg[BUF_SIZE];
        // Send an error message to the player
        snprintf(error_msg, BUF_SIZE, "ERROR:%s:%d\n", INVALID_MOVE_EMPTY, joueur);
        printf("error_msg: %s\n", error_msg);
        write_client(clients[client_index].sock, error_msg);
        return;
    }

    bool coup_valide = jouer_coup(jeu, joueur, case_depart);

    if (coup_valide)
    {   

        // Update of save
        if (partie->cout_index <= BUF_SAVE_SIZE - 1)
        {
            partie->cout[partie->cout_index] = ':';
            partie->cout_index++;
        }
        if (partie->cout_index <= BUF_SAVE_SIZE - 2)
        {
            // Update of save (care of the case where case_depart is greater than 9)
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


        // Update the tour
        partie->tour = (joueur == 1) ? 2 : 1;

        // Check if the game is over
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

            // Send the end message to the players
            for (int i = 0; i < actual; i++)
            {
                if (strcmp(clients[i].name, partie->awale_challenge.challenger) == 0 ||
                    strcmp(clients[i].name, partie->awale_challenge.challenged) == 0)
                {
                    //clients[i].partie_index = -1;
                    write_client(clients[i].sock, end_msg);
                }
            }

            // Send the end message to the spectators [Spectator]
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

            // Update of the ranking (we could have used the function update_elo_ratings)
            float part_of_calcul_challenged = (float) (classement_challenger - classement_challenged) / 400;
            float part_of_calcul_challenger = (float) (classement_challenged - classement_challenger) / 400;
            float p_vict_challenger = 1/(1+pow(10, part_of_calcul_challenger));
            float p_vict_challenged = 1/(1+pow(10, part_of_calcul_challenged));
            printf("ratio challenger : %f\n", p_vict_challenger);
            printf("ratio challenged : %f\n", p_vict_challenged);

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

            // In_save
            partie->in_save = true;

            return;
        }

        else
        {
            // Send the updated board state to the players
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
        // Invalid move for "Famine" rule
        char error_msg[BUF_SIZE];
        // Send an error message to the player for invalid move
        snprintf(error_msg, BUF_SIZE, "ERROR:%s:%d\n", INVALID_MOVE_FAMINE, joueur);
        write_client(clients[client_index].sock, error_msg);
    }
}

// Function to ask a player to save the game (and actions in consequences)
void handle_save(Client *clients, int actual, int client_index, const char *buffer)
{
    char response[BUF_SIZE];
    char *save = strdup(buffer);
    printf("save: %s\n", save);
    printf("buffer: %s\n", buffer);

    // Find the game
    int partie_index = clients[client_index].partie_index;
    if (partie_index == -1)
    {
        free(save);
        return;
    }

    PartieAwale *partie = &awale_parties[partie_index];
    
    // Find the players in the game
    Client *challenger = findClientByPseudo(clients, actual, partie->awale_challenge.challenger);
    Client *challenged = findClientByPseudo(clients, actual, partie->awale_challenge.challenged);
    
    if (!challenger || !challenged)
    {
        free(save);
        return;
    }

    // Save the response and mark that this player has responded
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
    clients[client_index].partie_index = -1;

    free(save);

    // If both players have responded (-1), we can clean up the game
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

// Function to handle a private message to someone
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
}

// Function to list all connected clients
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

// Function who try to accept a spectator in a "game" if the game exists and is not full/private
void handle_spec(Client *clients, int actual, int client_index, const char *buffer)
{
    printf("buffer: %s\n", buffer); //pseudo

    // Find the game
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


    // Add the client to the spectators
    partie->Spectators[partie->nbSpectators] = clients[client_index];
    partie->nbSpectators++;
    
    // Update the client
    clients[client_index].partie_index = player->partie_index;
    
    write_client(clients[client_index].sock, "You are now spectating this game\n");
    sleep(1);
    

    // Send the initial board state to the spectator
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

// Function to list all games in progress
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

// Function to list blocked players
void handle_list_blocked(Client *clients, int actual, int client_index)
{
    char list[BUF_SIZE * MAX_CLIENTS] = "Blocked users:\n";

    for (int i=0; i<clients[client_index].nbBlock; i++)
    {
        char user_entry[BUF_SIZE];
        snprintf(user_entry, BUF_SIZE, "- %s\n", clients[client_index].block[i]);
        strcat(list, user_entry);
    }

    if (clients[client_index].nbBlock == 0)
    {
        strcat(list, "0 blocked user\n");
    }

    write_client(clients[client_index].sock, list);
}

// Function to block a player
void handle_block(Client *clients, int actual, int client_index, const char *buffer)
{
    
    char target_pseudo[BUF_SIZE];
    strncpy(target_pseudo, buffer, BUF_SIZE - 1);
    target_pseudo[BUF_SIZE - 1] = '\0';

    if (strcmp(clients[client_index].name, target_pseudo) == 0)
    {
        write_client(clients[client_index].sock, "You cannot block yourself.\n");
        return;
    }

    for (int i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, target_pseudo) == 0)
        {
            for (int j = 0; j < clients[client_index].nbBlock; j++)
            {
                if (strcmp(clients[client_index].block[j], target_pseudo) == 0)
                {
                    write_client(clients[client_index].sock, "This player is already blocked.\n");
                    return;
                }
            }

            strncpy(clients[client_index].block[clients[client_index].nbBlock], target_pseudo, PSEUDO_MAX_LENGTH - 1);
            
            clients[client_index].block[clients[client_index].nbBlock][PSEUDO_MAX_LENGTH - 1] = '\0'; // Ensure null termination
            clients[client_index].nbBlock++;
            write_client(clients[client_index].sock, "Player blocked successfully.\n");
            return;
        }
    }

    write_client(clients[client_index].sock, "User not found.\n");
}

// Function to unblock a player
void handle_unblock(Client *clients, int actual, int client_index, const char *buffer)
{
    char target_pseudo[BUF_SIZE];
    strncpy(target_pseudo, buffer, BUF_SIZE - 1);
    target_pseudo[BUF_SIZE - 1] = '\0';

    for (int i = 0; i < clients[client_index].nbBlock; i++)
    {
        if (strcmp(clients[client_index].block[i], target_pseudo) == 0)
        {
            memmove(&clients[client_index].block[i], &clients[client_index].block[i + 1],
                    (clients[client_index].nbBlock - i - 1) * PSEUDO_MAX_LENGTH);
            clients[client_index].nbBlock--;
            write_client(clients[client_index].sock, "Player unblocked successfully.\n");
            return;
        }
    }

    write_client(clients[client_index].sock, "Player not found in blocked list.\n");
}

void handle_list_friend(Client *clients, int actual, int client_index)
{
    char list[BUF_SIZE * MAX_CLIENTS] = "Friends:\n";

    for (int i=0; i<clients[client_index].nbFriend; i++)
    {
        char user_entry[BUF_SIZE];
        snprintf(user_entry, BUF_SIZE, "- %s\n", clients[client_index].friend[i]);
        strcat(list, user_entry);
    }

    if (clients[client_index].nbFriend == 0)
    {
        strcat(list, "0 friend\n");
    }

    write_client(clients[client_index].sock, list);
}

void handle_friend(Client *clients, int actual, int client_index, const char *buffer)
{
    return;
}

void handle_unfriend(Client *clients, int actual, int client_index, const char *buffer)
{
    return;
}