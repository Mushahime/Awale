#include "commands.h"
#include "../awale.h"
#include <string.h>
#include <time.h>

// Due to a strange code architecture, some functions are too much imbriqued because we needed to always listen to the server...
// But we can still refactor some parts of the code to make it more readable and maintainable. But it works for now.

/**
 * @brief Handles sending a public message to the server.
 *
 * @param sock The socket connected to the server.
 */
void handle_send_public_message(SOCKET sock)
{
    char buffer[BUF_SIZE];
    printf("\033[1;34mEnter your message: \033[0m");
    if (fgets(buffer, sizeof(buffer) - 1, stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline
        write_server(sock, buffer);
    }
}

/**
 * @brief Handles sending a private message to a specific user.
 *
 * @param sock The socket connected to the server.
 */
void handle_send_private_message(SOCKET sock)
{
    char dest[PSEUDO_MAX_LENGTH];
    char msg[BUF_SIZE];
    char buffer[BUF_SIZE];

    printf("\033[1;34mEnter recipient's nickname: \033[0m");
    if (fgets(dest, sizeof(dest), stdin) == NULL)
        return;
    dest[strcspn(dest, "\n")] = '\0'; // Remove newline

    printf("\033[1;34mEnter your message: \033[0m");
    if (fgets(msg, sizeof(msg), stdin) == NULL)
        return;
    msg[strcspn(msg, "\n")] = '\0'; // Remove newline

    snprintf(buffer, sizeof(buffer), "mp:%s:%s", dest, msg);
    write_server(sock, buffer);
}

/**
 * @brief Handles requesting the list of users from the server.
 *
 * @param sock The socket connected to the server.
 */
void handle_list_users(SOCKET sock)
{
    char buffer[BUF_SIZE];
    strcpy(buffer, "list:");
    write_server(sock, buffer);
}

/**
 * @brief Allow to see the save of the game
 *
 */
/**
 * @brief Allow to see the save of the game
 */
void handle_save()
{
    if (save_count == 0)
    {
        printf("\n");
        printf("\033[1;31mNo save available.\033[0m\n");
        display_menu();
        return;
    }

    for (int i = 0; i < save_count; i++)
    {
        printf("\033[1;34mSave %d:\033[0m\n", i + 1);

        char temp[BUF_SAVE_SIZE];
        strncpy(temp, save[i], BUF_SAVE_SIZE);
        temp[BUF_SAVE_SIZE - 1] = '\0';

        char *save_ptr;
        char *save_date = strtok_r(temp, "_", &save_ptr);
        printf("Date: %s -> ", save_date);

        char *challenger = strtok_r(NULL, ":", &save_ptr);
        printf("Challenger : %s VS ", challenger);

        char *challenged = strtok_r(NULL, ":", &save_ptr);
        printf("Challenged : %s\n", challenged);
    }

    printf("\033[1;34mEnter the number of the save you want to load: \033[0m");
    fflush(stdout);

    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    while (1)
    {
        if (partie_en_cours)
        {
            return;
        }

        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(STDIN_FILENO + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                int save_choice = atoi(input);
                if (save_choice > 0 && save_choice <= save_count)
                {
                    save_index = save_choice - 1;
                    printf("\033[1;32mSave loaded successfully!\033[0m\n");
                    printf("save: %s\n", save[save_index]);
                    demo_partie(save[save_index]);
                    display_menu();
                    return;
                }
                else
                {
                    printf("\n");
                    printf("\033[1;31mInvalid save choice.\033[0m\n");
                    printf("\033[1;31mReturning to main menu...\033[0m\n");
                    display_menu();
                    return;
                }
            }
        }

        if (partie_en_cours)
        {
            return;
        }
    }
}

void demo_partie(const char *buffer)
{
    char *temp_buffer = malloc(strlen(buffer) + 2); // for the ':' and '\0'
    if (!temp_buffer)
    {
        perror("Error allocating memory");
        return;
    }

    strcpy(temp_buffer, buffer);
    strcat(temp_buffer, ":");

    char *saveptr;
    char *date = strtok_r(temp_buffer, "_", &saveptr);
    char *challenger = strtok_r(NULL, ":", &saveptr);
    char *challenged = strtok_r(NULL, ":", &saveptr);

    printf("date: %s\n", date);
    printf("challenger (joueur 1): %s\n", challenger);
    printf("challenged (joueur 2): %s\n", challenged);

    JeuAwale jeu;
    initialiser_plateau(&jeu);
    afficher_plateau(&jeu);

    char *tour = strtok_r(NULL, ":", &saveptr);
    int tour_int = atoi(tour);

    if (tour_int == 1)
    {
        printf("It's %s who started (0-5)\n", challenger);
    }
    else
    {
        printf("It's %s who started (6-11)\n", challenged);
    }

    sleep(2);
    char *coup = strtok_r(NULL, ":", &saveptr);

    while (coup != NULL)
    {
        int coup_int = atoi(coup);
        printf("\n");
        printf("--------------------------------\n");
        printf("Shout played: %d\n", coup_int);
        jouer_coup(&jeu, tour_int, coup_int);
        afficher_plateau(&jeu);
        sleep(2);
        coup = strtok_r(NULL, ":", &saveptr);
        tour_int = (tour_int == 1) ? 2 : 1;
    }
    printf("--------------------------------\n");
    printf("\n");
    printf("End of the game\n");
    int score_J2 = jeu.plateau[6] + jeu.plateau[7] + jeu.plateau[8] + jeu.plateau[9] + jeu.plateau[10] + jeu.plateau[11] + jeu.score_joueur1;
    int score_J1 = jeu.plateau[0] + jeu.plateau[1] + jeu.plateau[2] + jeu.plateau[3] + jeu.plateau[4] + jeu.plateau[5] + jeu.score_joueur2;
    printf("Final score: P1: %d, P2: %d\n", score_J1, score_J2);

    if (score_J1 == score_J2)
    {
        printf("Draw!\n");
    }
    else
    {
        printf("The winner is %s!\n", (score_J1 > score_J2) ? challenger : challenged);
    }

    free(temp_buffer);
}

/**
 * @brief Handles the bio options menu.
 *
 * @param sock The socket connected to the server.
 */
void handle_bio_options(SOCKET sock)
{
    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    printf("\n\033[1;36m=== Bio Options ===\033[0m\n");
    printf("1. Set your bio\n");
    printf("2. View someone's bio\n");
    printf("Others. Return to main menu\n");
    printf("Choice: ");
    fflush(stdout);

    while (1)
    {
        if (partie_en_cours)
        {
            return;
        }

        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0';
                int bio_choice = atoi(input);

                if (bio_choice == 1)
                {
                    char bio[MAX_BIO_LENGTH] = "setbio:";
                    printf("\033[1;34mEnter your bio:\033[0m\n");
                    get_multiline_input(bio + 7, sizeof(bio) - 7);
                    write_server(sock, bio);
                    return;
                }
                else if (bio_choice == 2)
                {
                    printf("\033[1;34mEnter nickname to view bio: \033[0m");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        char buffer[BUF_SIZE];
                        snprintf(buffer, sizeof(buffer), "getbio:%s", input);
                        write_server(sock, buffer);
                        return;
                    }
                }
                else
                {
                    printf("\n");
                    printf("\033[1;31mInvalid bio option selected.\033[0m\n");
                    printf("\033[1;31mReturning to main menu...\033[0m\n");
                    display_menu();
                    return;
                }
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char buffer[BUF_SIZE];
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            printf("\n");
            handle_server_message(sock, buffer);

            if (!partie_en_cours)
            {
                printf("\n\033[1;36m=== Bio Options ===\033[0m\n");
                printf("1. Set your bio\n");
                printf("2. View someone's bio\n");
                printf("Others. Return to main menu\n");
                printf("Choice: ");
                fflush(stdout);
            }
        }
    }
}

/**
 * @brief Handles the block menu.
 *
 * @param sock The socket connected to the server.
 */
void handle_block(SOCKET sock)
{
    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    printf("\n\033[1;36m=== Block Options ===\033[0m\n");
    printf("1. Block somebody\n");
    printf("2. Remove somebody\n");
    printf("3. List blocked users\n");
    printf("Others. Return to main menu\n");
    printf("Choice: ");
    fflush(stdout);

    while (1)
    {
        if (partie_en_cours)
        {
            return;
        }

        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0';
                int bio_choice = atoi(input);

                if (bio_choice == 1)
                {
                    printf("\033[1;34mEnter the nickname to add to the list: \033[0m");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        char buffer[BUF_SIZE];
                        snprintf(buffer, sizeof(buffer), "block:%s", input);
                        write_server(sock, buffer);
                        return;
                    }
                }
                else if (bio_choice == 2)
                {
                    printf("\033[1;34mEnter the nickname to remove to the list: \033[0m");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        char buffer[BUF_SIZE];
                        snprintf(buffer, sizeof(buffer), "unblock:%s", input);
                        write_server(sock, buffer);
                        return;
                    }
                }
                else if (bio_choice == 3)
                {
                    char buffer[BUF_SIZE];
                    strcpy(buffer, "list_blocked:");
                    write_server(sock, buffer);
                    return;
                }
                else
                {
                    printf("\n");
                    printf("\033[1;31mInvalid bio option selected.\033[0m\n");
                    printf("\033[1;31mReturning to main menu...\033[0m\n");
                    display_menu();
                    return;
                }
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char buffer[BUF_SIZE];
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            printf("\n");
            handle_server_message(sock, buffer);

            if (!partie_en_cours)
            {
                printf("\n\033[1;36m=== Block Options ===\033[0m\n");
                printf("1. Block somebody\n");
                printf("2. Remove somebody\n");
                printf("3. List blocked users\n");
                printf("Others. Return to main menu\n");
                printf("Choice: ");
                fflush(stdout);
            }
        }
    }
}

/**
 * @brief Handles the block menu.
 *
 * @param sock The socket connected to the server.
 */
void handle_friend(SOCKET sock)
{
    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    printf("\n\033[1;36m=== Friend Options ===\033[0m\n");
    printf("1. Friend somebody\n");
    printf("2. Remove someone\n");
    printf("3. Friends list\n");
    printf("Others. Return to main menu\n");
    printf("Choice: ");
    fflush(stdout);

    while (1)
    {
        if (partie_en_cours)
        {
            return;
        }

        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0';
                int bio_choice = atoi(input);

                if (bio_choice == 1)
                {
                    printf("\033[1;34mEnter the nickname to add to the list: \033[0m");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        char buffer[BUF_SIZE];
                        snprintf(buffer, sizeof(buffer), "friend:%s", input);
                        write_server(sock, buffer);
                        return;
                    }
                }
                else if (bio_choice == 2)
                {
                    printf("\033[1;34mEnter the nickname to remove to the list: \033[0m");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        char buffer[BUF_SIZE];
                        snprintf(buffer, sizeof(buffer), "unfriend:%s", input);
                        write_server(sock, buffer);
                        return;
                    }
                }
                else if (bio_choice == 3)
                {
                    char buffer[BUF_SIZE];
                    strcpy(buffer, "list_friend:");
                    write_server(sock, buffer);
                    return;
                }
                else
                {
                    printf("\n");
                    printf("\033[1;31mInvalid bio option selected.\033[0m\n");
                    printf("\033[1;31mReturning to main menu...\033[0m\n");
                    display_menu();
                    return;
                }
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char buffer[BUF_SIZE];
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            printf("\n");
            handle_server_message(sock, buffer);

            if (!partie_en_cours)
            {
                printf("\n\033[1;36m=== Friend Options ===\033[0m\n");
                printf("1. Friend somebody\n");
                printf("2. Remove someone\n");
                printf("3. Friends list\n");
                printf("Others. Return to main menu\n");
                printf("Choice: ");
                fflush(stdout);
            }
        }
    }
}

/**
 * @brief Handles the spec menu.
 *
 * @param sock The socket connected to the server.
 */
void handle_spec(SOCKET sock)
{
    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    printf("\n\033[1;36m=== Spec Options ===\033[0m\n");
    printf("Tap 1. Join a game\n");
    printf("Or return to main menu (input != 1)\n");
    printf("Choice: ");
    fflush(stdout);

    while (1)
    {
        if (partie_en_cours)
        {
            return;
        }

        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0';
                int spec_choice = atoi(input);

                if (spec_choice == 1)
                {
                    printf("\033[1;34mEnter the nickname of the player you want to spectate: \033[0m");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        char buffer[BUF_SIZE];
                        snprintf(buffer, sizeof(buffer), "spec:%s", input);
                        write_server(sock, buffer);
                        return;
                    }
                }
                else
                {
                    printf("\n");
                    printf("\033[1;31mInvalid choice.\033[0m\n");
                    printf("\033[1;31mReturning to main menu...\033[0m\n");
                    display_menu();
                    return;
                }
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char buffer[BUF_SIZE];
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            printf("\n");
            handle_server_message(sock, buffer);

            if (!partie_en_cours)
            {
                printf("\n\033[1;36m=== Spec Options ===\033[0m\n");
                printf("Tap 1. Join a game\n");
                printf("Or return to main menu (input != 1)\n");
                printf("Choice: ");
                fflush(stdout);
            }
        }
    }
}

// active listening for each fgets (that is why imbricated structure of code)
/**
 * @brief Handles initiating a game of Awale.
 *
 * @param sock The socket connected to the server.
 */
void handle_play_awale(SOCKET sock)
{
    fd_set rdfs;
    struct timeval tv;
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];
    char private_game[BUF_SIZE];
    char private_player[BUF_SIZE];
    //waiting_for_response = true;

    printf("\033[1;34mEnter the nickname of the player you want to play against: \033[0m");
    fflush(stdout);

    while (1)
    {
        if (partie_en_cours)
        {
            return;
        }

        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(buffer, sizeof(buffer), stdin) != NULL)
            {
                buffer[strcspn(buffer, "\n")] = '\0';

                // Now ask about private game
                printf("\033[1;34mDo you want to private the game? (yes/no)\033[0m\n");
                fflush(stdout);

                while (1)
                {
                    FD_ZERO(&rdfs);
                    FD_SET(STDIN_FILENO, &rdfs);
                    FD_SET(sock, &rdfs);

                    tv.tv_sec = 0;
                    tv.tv_usec = 100000;

                    ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
                    if (ret == -1)
                    {
                        perror("select()");
                        return;
                    }

                    if (FD_ISSET(STDIN_FILENO, &rdfs))
                    {
                        if (fgets(private_game, sizeof(private_game), stdin) != NULL)
                        {
                            private_game[strcspn(private_game, "\n")] = '\0';

                            if (strcmp(private_game, "yes") == 0)
                            {
                                printf("\033[1;34mEnter the list of players who can spectate the game (: separated), or press Enter to use your friend list\033[0m\n");
                                fflush(stdout);

                                while (1)
                                {
                                    FD_ZERO(&rdfs);
                                    FD_SET(STDIN_FILENO, &rdfs);
                                    FD_SET(sock, &rdfs);

                                    tv.tv_sec = 0;
                                    tv.tv_usec = 100000;

                                    ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
                                    if (ret == -1)
                                    {
                                        perror("select()");
                                        return;
                                    }

                                    if (FD_ISSET(STDIN_FILENO, &rdfs))
                                    {
                                        if (fgets(private_player, sizeof(private_player), stdin) != NULL)
                                        {
                                            private_player[strcspn(private_player, "\n")] = '\0';

                                            // If empty input, request friend list from server
                                            if (strlen(private_player) == 0)
                                            {
                                                write_server(sock, "list_friend:");

                                                // Read server response
                                                char friend_list[BUF_SIZE];
                                                if (read_server(sock, friend_list) != -1)
                                                {
                                                    // Parse friend list response and extract names
                                                    char *line = strtok(friend_list, "\n");
                                                    char friends[BUF_SIZE] = "";

                                                    while ((line = strtok(NULL, "\n")) != NULL)
                                                    {
                                                        if (line[0] == '-')
                                                        {
                                                            // Skip the "- " prefix
                                                            strcat(friends, line + 2);
                                                            strcat(friends, ":");
                                                        }
                                                    }

                                                    // Remove trailing colon if exists
                                                    size_t len = strlen(friends);
                                                    if (len > 0 && friends[len - 1] == ':')
                                                    {
                                                        friends[len - 1] = '\0';
                                                    }

                                                    snprintf(input, sizeof(input), "awale:%s:%s:%s:", buffer, private_game, friends);
                                                }
                                            }
                                            else
                                            {
                                                snprintf(input, sizeof(input), "awale:%s:%s:%s:", buffer, private_game, private_player);
                                            }
                                            write_server(sock, input);
                                            printf("Waiting for the other player to accept...\n");
                                            return;
                                        }
                                    }

                                    if (FD_ISSET(sock, &rdfs))
                                    {
                                        char server_buffer[BUF_SIZE];
                                        int n = read_server(sock, server_buffer);
                                        if (n == 0)
                                        {
                                            printf("\n\033[1;31mServer disconnected!\033[0m\n");
                                            exit(errno);
                                        }
                                        handle_server_message(sock, server_buffer);
                                        printf("\033[1;34mEnter the list of players who can spectate the game (: separated), or press Enter to use your friend list\033[0m\n");
                                        fflush(stdout);
                                    }
                                }
                            }
                            else
                            {
                                snprintf(input, sizeof(input), "awale:%s:%s", buffer, private_game);
                                write_server(sock, input);
                                printf("Waiting for the other player to accept...\n");
                                waiting_for_response = true;
                                return;
                            }
                        }
                    }

                    if (FD_ISSET(sock, &rdfs))
                    {
                        char server_buffer[BUF_SIZE];
                        int n = read_server(sock, server_buffer);
                        if (n == 0)
                        {
                            printf("\n\033[1;31mServer disconnected!\033[0m\n");
                            exit(errno);
                        }
                        handle_server_message(sock, server_buffer);
                        printf("\033[1;34mDo you want to private the game? (yes/no)\033[0m\n");
                        fflush(stdout);
                    }
                }
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char server_buffer[BUF_SIZE];
            int n = read_server(sock, server_buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            handle_server_message(sock, server_buffer);
            printf("\033[1;34mEnter the nickname of the player you want to play against: \033[0m");
            fflush(stdout);
        }
    }
}

/**
 * @brief Handles requesting the list of games in progress from the server.
 *
 * @param sock The socket connected to the server.
 */
void handle_list_games(SOCKET sock)
{
    char buffer[BUF_SIZE];
    strcpy(buffer, "awale_list:");
    write_server(sock, buffer);

    // Attendre la réponse du serveur pour la liste des parties
    if (read_server(sock, buffer) == -1)
    {
        return;
    }

    // Si aucune partie en cours, retourner directement
    if (strstr(buffer, "0 game") != NULL)
    {
        printf("\033[1;32m%s\033[0m", buffer);
        display_menu();
        return;
    }

    printf("\033[1;32m%s\033[0m", buffer);
    printf("\n\033[1;36m=== Games Options ===\033[0m\n");
    printf("1. Spectate a game\n");
    printf("Others. Return to main menu\n");
    printf("Choice: ");
    fflush(stdout);

    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    while (1)
    {
        if (partie_en_cours)
        {
            return; // Sort directement si une partie a commencé
        }

        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0';
                int choice = atoi(input);

                if (choice == 1)
                {
                    printf("\033[1;34mEnter the nickname of the player you want to spectate: \033[0m");
                    if (fgets(input, sizeof(input), stdin) != NULL)
                    {
                        input[strcspn(input, "\n")] = '\0';
                        char buffer[BUF_SIZE];
                        snprintf(buffer, sizeof(buffer), "spec:%s", input);
                        write_server(sock, buffer);
                        return;
                    }
                }
                else
                {
                    printf("\n");
                    printf("\033[1;31mReturning to main menu...\033[0m\n");
                    display_menu();
                    return;
                }
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char buffer[BUF_SIZE];
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            printf("\n");
            handle_server_message(sock, buffer);

            if (!partie_en_cours)
            {
                printf("\033[1;32m%s\033[0m", buffer);
                printf("\n\033[1;36m=== Games Options ===\033[0m\n");
                printf("1. Spectate a game\n");
                printf("Others. Return to main menu\n");
                printf("Choice: ");
                fflush(stdout);
            }
        }
    }
}

/**
 * @brief Handles quitting the application.
 */
void handle_quit()
{
    printf("\033[1;33mGoodbye!\033[0m\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief saves the game
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the server message.
 */
void saver(SOCKET sock, char *buffer)
{
    // Send to the server if u want to save the game
    char save_game[BUF_SIZE];
    printf("Do you want to save the game? (yes/no)\n");
    // envoyer le message au serveur
    if (fgets(save_game, sizeof(save_game), stdin) != NULL)
    {
        // forme save:pseudo:yes/no
        save_game[strcspn(save_game, "\n")] = '\0'; // Remove newline
        char save_game_message[BUF_SIZE];
        snprintf(save_game_message, sizeof(save_game_message), "save:%s", save_game);
        write_server(sock, save_game_message);

        // wait for the response
        char response[BUF_SIZE];
        if (read_server(sock, response) == -1)
        {
            return;
        }

        if (response[0] == '-')
        {
            printf("The game is not saved\n");
            return;
        }

        // Add the date before the response (day/month/year hour) to response
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        char date[BUF_SIZE];
        snprintf(date, sizeof(date), "%d/%d/%d %d:%d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);

        char total[BUF_SAVE_SIZE + BUF_SIZE];
        snprintf(total, sizeof(total), "%s_%s", date, response);

        strncpy(save[save_count], total, BUF_SAVE_SIZE);
        save[save_count][strlen(total)] = '\0';

        printf("save ok\n");

        save_index = (save_index + 1) % MAX_PARTIES;
        if (save_count < MAX_PARTIES)
        {
            save_count++;
        }
    }
}

/**
 * @brief Processes Awale game messages from the server.
 *
 * @param sock The socket connected to the server.
 * @param msg_body The body of the Awale message excluding the "AWALE:" prefix.
 */
void process_awale_message(SOCKET sock, char *msg_body)
{
    partie_en_cours = true;
    int plateau[TAILLE_PLATEAU];
    int joueur;
    char nom[PSEUDO_MAX_LENGTH];

    // Duplicate the message to avoid modifying the original buffer
    char *msg = strdup(msg_body);
    if (msg == NULL)
    {
        perror("strdup failed");
        free(msg);
        exit(EXIT_FAILURE);
    }

    char *token = strtok(msg, ":");
    int i = 0;

    // Retrieve the state of the board
    while (token != NULL && i < TAILLE_PLATEAU)
    {
        plateau[i++] = atoi(token);
        token = strtok(NULL, ":");
    }

    // Retrieve the player's nickname
    if (token != NULL)
    {
        strncpy(nom, token, sizeof(nom) - 1);
        nom[sizeof(nom) - 1] = '\0';
        token = strtok(NULL, ":");
    }

    // Retrieve the player's number
    if (token != NULL)
    {
        joueur = atoi(token);
    }

    // Retrieve the scores of the players
    int score_joueur1 = atoi(strtok(NULL, ":"));
    int score_joueur2 = atoi(strtok(NULL, ":"));

    free(msg); // Free the duplicated message

    // Display the game board
    afficher_plateau2(plateau, score_joueur1, score_joueur2);

    // Determine if it's the user's turn
    if (strcmp(nom, pseudo) == 0)
    {
        prompt_for_move(sock, joueur, nom, plateau, score_joueur1, score_joueur2);
    }
    else
    {
        printf("\nWaiting for %s's move...\n", nom);
        printf("You can send a private message using 'mp:pseudo:message'\n");
        printf("Type 'quit' to leave the game\n");
    }
}

/**
 * @brief Prompts the user to make a move in the Awale game.
 *
 * @param sock The socket connected to the server.
 * @param joueur The player's number.
 * @param nom The player's nickname.
 * @param plateau The current state of the game board.
 * @param score_joueur1 The score of player 1.
 * @param score_joueur2 The score of player 2.
 */
void prompt_for_move(SOCKET sock, int joueur, const char *nom, int plateau[], int score_joueur1, int score_joueur2)
{
    int first = (joueur == 1) ? 0 : 6;
    int last = (joueur == 1) ? 5 : 11;

    printf("\nIt's your turn, player %d!\n", joueur);
    printf("Enter a number between %d and %d to play\n", first, last);
    printf("Type 'mp:pseudo:message' to send a private message\n");
    printf("Type 'quit' to leave the game\n");

    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    while (partie_en_cours)
    { // Changé while(1) en while(partie_en_cours)
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0';

                if (!partie_en_cours)
                { // Vérifier si la partie est toujours en cours avant de traiter l'input
                    return;
                }

                if (strcmp(input, "quit") == 0)
                {
                    handle_quit_game(sock);
                    return;
                }

                if (strncmp(input, "mp:", 3) == 0)
                {
                    char *rest = input + 3;
                    char *pseudo = strchr(rest, ':');

                    if (!pseudo)
                    {
                        printf("\033[1;31mInvalid format! Use 'mp:pseudo:message'\033[0m\n");
                        continue;
                    }

                    int pseudo_len = pseudo - rest;
                    if (pseudo_len < PSEUDO_MIN_LENGTH || pseudo_len >= PSEUDO_MAX_LENGTH)
                    {
                        printf("\033[1;31mInvalid nickname length\033[0m\n");
                        continue;
                    }

                    if (strlen(pseudo + 1) == 0)
                    {
                        printf("\033[1;31mMessage cannot be empty\033[0m\n");
                        continue;
                    }

                    write_server(sock, input);
                    printf("\033[1;32mMessage sent!\033[0m\n");
                    printf("Enter a number between %d and %d to play\n", first, last);
                    printf("Type 'mp:pseudo:message' to send a private message\n");
                    printf("Type 'quit' to leave the game\n");
                    continue;
                }

                int move_int = atoi(input);
                if (move_int >= first && move_int <= last)
                {
                    char buffer[BUF_SIZE];
                    snprintf(buffer, sizeof(buffer), "awale_move:%d", move_int);
                    write_server(sock, buffer);
                    return;
                }
                else
                {
                    printf("\033[1;31mInvalid input!\033[0m\n");
                }
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char buffer[BUF_SIZE];
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            handle_server_message(sock, buffer);

            if (partie_en_cours)
            {
                printf("Enter a number between %d and %d to play\n", first, last);
                printf("Type 'mp:pseudo:message' to send a private message\n");
                printf("Type 'quit' to leave the game\n");
            }
            else
            {
                return;
            }
        }
    }
}

/**
 * @brief Handles quitting the Awale game.
 *
 * @param sock The socket connected to the server.
 */
void handle_quit_game(SOCKET sock)
{
    char buffer[BUF_SIZE];
    snprintf(buffer, sizeof(buffer), "quit_game:");
    write_server(sock, buffer);
    partie_en_cours = false;
    printf("\033[1;31mYou left the game\033[0m\n");
    display_menu();
}

/**
 * @brief Processes error messages from the server.
 *
 * @param buffer The buffer containing the error message.
 */
void process_error_message(SOCKET sock, char *buffer)
{
    char *error_type = strtok(buffer, ":");
    char *error_message = strtok(NULL, ":");
    char *joueur_str = strtok(NULL, ":");
    int joueur = (joueur_str != NULL) ? atoi(joueur_str) : 0;

    printf("\033[1;31m%s: %s\033[0m\n", error_type, error_message);

    prompt_for_new_move(sock, joueur);
}

/**
 * @brief Prompts the user to enter a new move after an error.
 *
 * @param joueur The player's number.
 */
void prompt_for_new_move(SOCKET sock, int joueur)
{
    int first = (joueur == 1) ? 0 : 6;
    int last = (joueur == 1) ? 5 : 11;

    printf("Enter a valid move (%d-%d): \n", first, last);
    printf("Others commands: 'mp:pseudo:message' or 'quit' to leave the game\n");
    fflush(stdout);

    fd_set rdfs;
    struct timeval tv;
    char input[BUF_SIZE];

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ret = select(sock + 1, &rdfs, NULL, NULL, &tv);
        if (ret == -1)
        {
            perror("select()");
            return;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0';
                int move = atoi(input);
                if (move >= first && move <= last)
                {
                    char buffer[BUF_SIZE];
                    snprintf(buffer, sizeof(buffer), "awale_move:%d", move);
                    write_server(sock, buffer);
                    return;
                }
                printf("\033[1;31mInvalid move!\033[0m\n");
                printf("Enter a valid move (%d-%d): ", first, last);
                fflush(stdout);
            }
        }

        if (FD_ISSET(sock, &rdfs))
        {
            char buffer[BUF_SIZE];
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\n\033[1;31mServer disconnected!\033[0m\n");
                exit(errno);
            }
            handle_server_message(sock, buffer);
            printf("\nEnter a valid move (%d-%d): ", first, last);
            fflush(stdout);
        }
    }
}

/**
 * @brief Processes fight/challenge messages from the server.
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the fight message.
 */
bool process_fight_message(SOCKET sock, char *buffer)
{
    printf("\033[1;31m%s\033[0m\n", buffer); // Red for fight messages
    printf("\033[1;31mDo you accept the challenge? (yes/no)\033[0m\n");

    char response[BUF_SIZE];
    while (1)
    {
        if (fgets(response, sizeof(response), stdin) != NULL)
        {
            response[strcspn(response, "\n")] = '\0'; // Remove newline

            if (strcmp(response, "yes") == 0 || strcmp(response, "no") == 0)
            {
                char challenge_response[BUF_SIZE];
                snprintf(challenge_response, sizeof(challenge_response), "awale_response:%s", response);
                write_server(sock, challenge_response);
                waiting_for_response = false;  // Reset waiting_for_response flag
                if (strcmp(response, "no") == 0) {
                    display_menu();  // Show menu immediately after declining
                }
                break;
            }
            else
            {
                printf("\033[1;31mPlease enter 'yes' or 'no'\033[0m\n");
            }
        }
    }

    if (strcmp(response, "yes") == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Processes fight/challenge messages from the server.
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the fight message.
 */
void process_friend_message(SOCKET sock, char *buffer)
{
    printf("\033[1;31m%s\033[0m\n", buffer);
    printf("\033[1;31mDo you accept the friend request? (yes/no)\033[0m\n");

    // form buffer [Friend] Friend_request from :pseudo
    char *temp = strtok(buffer, ":");
    char *pseudo_ask = strtok(NULL, ":");

    char response[BUF_SIZE];
    while (1)
    {
        if (fgets(response, sizeof(response), stdin) != NULL)
        {
            response[strcspn(response, "\n")] = '\0'; // Remove newline

            if (strcmp(response, "yes") == 0 || strcmp(response, "no") == 0)
            {
                char challenge_response[BUF_SIZE];
                snprintf(challenge_response, sizeof(challenge_response), "friend_response:%s:%s", response, pseudo_ask);
                write_server(sock, challenge_response);

                // If in a game, redisplay the game prompt
                if (partie_en_cours)
                {
                    printf("\n");
                    printf("You can still play the game\n");
                    printf("Type 'mp:pseudo:message' to send a private message\n");
                    printf("Type 'quit' to leave the game\n");
                }
                break;
            }
            else
            {
                printf("\033[1;31mPlease enter 'yes' or 'no'\033[0m\n");
            }
        }
    }
}
/**
 * @brief Processes game over messages from the server.
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the game over message.
 */
void process_game_over_message(SOCKET sock, char *buffer)
{
    printf("\033[1;33m%s\033[0m\n", buffer);
    partie_en_cours = false;
}

/**
 * @brief Processes private messages from the server.
 *
 * @param buffer The buffer containing the private message.
 */
void process_private_message(char *buffer)
{
    fflush(stdout);
    printf("\n");
    printf("\033[1;35m%s\033[0m", buffer); // Magenta for private messages
}

/**
 * @brief Processes system messages like user join or disconnect.
 *
 * @param buffer The buffer containing the system message.
 */
void process_system_message(char *buffer)
{
    printf("\033[1;33m%s\033[0m\n", buffer); // Yellow for system messages
}

/**
 * @brief Processes challenge messages from the server.
 *
 * @param buffer The buffer containing the challenge message.
 */
void process_challenge_message(char *buffer)
{
    fflush(stdout);
    printf("\033[1;33m%s\033[0m\n", buffer); // Yellow for challenge messages
    if (strstr(buffer, "Game started") != NULL)
    {
        partie_en_cours = true; // Set game state before announcing wait
        printf("\033[1;33mStarting game in 5 seconds...\033[0m\n");
#ifdef WIN32
        Sleep(3000); // Windows Sleep in milliseconds
#else
        sleep(3); // Unix sleep in seconds
#endif
    }
}