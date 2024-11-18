// client2.c

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "client2.h"
#include "../awale.h"
#include <sys/select.h>

// Constants
#define MAX_INPUT_LENGTH 256

// Enumerations for Menu Choices
typedef enum
{
    SEND_PUBLIC_MESSAGE = 1,
    SEND_PRIVATE_MESSAGE,
    LIST_USERS,
    BIO_OPTIONS,
    PLAY_AWALE,
    LIST_GAMES_IN_PROGRESS,
    SEE_SAVE,
    CLEAR_SCREEN,
    QUIT
} MenuChoice;

int main(int argc, char **argv)
{
    if (argc > 3 || argc < 3 || atoi(argv[2]) < 1024 || atoi(argv[2]) > 65535)
    { // Ensure both address and name are provided
        printf("Error: arguments error\n");
        return EXIT_FAILURE;
    }
    printf("init OK\n");

    init();
    app(argv[1], atoi(argv[2]));
    end();

    return EXIT_SUCCESS;
}

/**
 * @brief Prompts the user for multiline input, ending with an empty line.
 *
 * @param buffer Pointer to the buffer where input will be stored.
 * @param max_size Maximum size of the buffer.
 */
void get_multiline_input(char *buffer, int max_size)
{
    printf("Enter your text (end with an empty line):\n");
    memset(buffer, 0, max_size);
    char line[MAX_INPUT_LENGTH];
    int total_len = 0;

    while (1)
    {
        if (fgets(line, sizeof(line), stdin) == NULL)
            break;
        if (line[0] == '\n')
            break;

        int line_len = strlen(line);
        if (total_len + line_len >= max_size - 1)
            break;

        strcat(buffer, line);
        total_len += line_len;
    }
}

/**
 * @brief Validates and sets the user's nickname by communicating with the server.
 *
 * @param sock The socket connected to the server.
 * @return int Returns 1 on success, 0 on failure.
 */
int get_valid_pseudo(SOCKET sock)
{
    char buffer[BUF_SIZE];
    char response[BUF_SIZE];

    while (1)
    {
        printf("\033[1;33mEnter your nickname (2-%d characters, letters, numbers, underscore only): \033[0m",
               PSEUDO_MAX_LENGTH - 1);
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            return 0;
        }

        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

        write_server(sock, buffer); // Send nickname to server

        if (read_server(sock, response) == -1)
        {
            return 0;
        }

        if (strcmp(response, "connected") == 0)
        {
            printf("\033[1;32mConnected successfully!\033[0m\n");
            strncpy(pseudo, buffer, PSEUDO_MAX_LENGTH - 1);
            pseudo[PSEUDO_MAX_LENGTH - 1] = '\0'; // Ensure null-termination
            return 1;
        }
        else
        {
            // Handle various error responses from the server
            if (strcmp(response, "pseudo_exists") == 0)
            {
                printf("\033[1;31mThis nickname is already taken. Please choose another one.\033[0m\n");
            }
            else if (strcmp(response, "pseudo_too_short") == 0)
            {
                printf("\033[1;31mNickname too short (minimum 2 characters).\033[0m\n");
            }
            else if (strcmp(response, "pseudo_too_long") == 0)
            {
                printf("\033[1;31mNickname too long (maximum %d characters).\033[0m\n", PSEUDO_MAX_LENGTH - 1);
            }
            else
            {
                printf("\033[1;31mInvalid nickname format. Use letters, numbers, and underscore only.\033[0m\n");
            }
            close(sock);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

/**
 * @brief Handles user input based on menu choices.
 *
 * @param sock The socket connected to the server.
 */
void handle_user_input(SOCKET sock)
{
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];
    int choice;


    if (fgets(input, sizeof(input), stdin) == NULL)
        return;

    if (partie_en_cours)
    {
        if (strncmp(input, "mp:", 3) == 0)
        {
            input[strcspn(input, "\n")] = '\0';
            write_server(sock, input);
        }
        else if (strncmp(input, "\n", 1) != 0) // Ignore les lignes vides
        {
            printf("During a game, you can only send private messages using 'mp:pseudo:message'\n");
        }
        return;
    }
    choice = atoi(input);

    switch (choice)
    {
    case SEND_PUBLIC_MESSAGE:
        handle_send_public_message(sock);
        display_menu();
        break;

    case SEND_PRIVATE_MESSAGE:
        handle_send_private_message(sock);
        break;

    case LIST_USERS:
        handle_list_users(sock);
        break;

    case BIO_OPTIONS:
        handle_bio_options(sock);
        break;

    case PLAY_AWALE:
        handle_play_awale(sock);
        break;

    case LIST_GAMES_IN_PROGRESS:
        handle_list_games(sock);
        break;

    case SEE_SAVE:
        handle_save();
        break;

    case CLEAR_SCREEN:
        clear_screen_custom();
        break;

    case QUIT:
        handle_quit();
        break;

    default:
        printf("\033[1;31mInvalid choice.\033[0m\n");
        break;
    }
}

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
void handle_save()
{
    if (save_count == 0)
    {
        printf("\033[1;31mNo save available.\033[0m\n");
        display_menu();
        return;
    }

    for (int i = 0; i < save_count; i++)
    {
        printf("\033[1;34mSave %d:\033[0m\n", i + 1);

        // Créer une copie temporaire du save pour le strtok
        char temp[BUF_SAVE_SIZE];
        strncpy(temp, save[i], BUF_SAVE_SIZE);
        temp[BUF_SAVE_SIZE-1] = '\0';

        char *save_ptr;  // Pour strtok_r
        char *save_date = strtok_r(temp, "_", &save_ptr);
        printf("Date: %s -> ", save_date);

        char *challenger = strtok_r(NULL, ":", &save_ptr);
        printf("Challenger : %s VS ", challenger);

        char *challenged = strtok_r(NULL, ":", &save_ptr);
        printf("Challenged : %s\n", challenged);
    }

    // Ask the user if he wants to load a save
    char input[BUF_SIZE];
    printf("\033[1;34mEnter the number of the save you want to load: \033[0m");
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
            printf("\033[1;31mInvalid save choice.\033[0m\n");
            printf("\033[1;33mReturning to main menu...\033[0m\n");
            display_menu();
            return;
        }
    }
}

void demo_partie(const char *buffer) {
    char *temp_buffer = malloc(strlen(buffer) + 2); // +2 pour ':' et '\0'
    if (!temp_buffer) {
        perror("Erreur allocation mémoire");
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
    
    if (tour_int == 1) {
        printf("C'est %s qui a commencé (0-5)\n", challenger);
    } else {
        printf("C'est %s qui a commencé (6-11)\n", challenged);
    }
    
    sleep(2);
    char *coup = strtok_r(NULL, ":", &saveptr);
    
    while (coup != NULL) {
        int coup_int = atoi(coup);
        printf("\n");
        printf("Coup joué: %d\n", coup_int);
        jouer_coup(&jeu, tour_int, coup_int);
        afficher_plateau(&jeu);
        sleep(2);
        coup = strtok_r(NULL, ":", &saveptr);
        tour_int = (tour_int == 1) ? 2 : 1;
    }
    printf("\n");
    printf("Fin de la partie\n");
    printf("Score final: J1: %d, J2: %d\n", jeu.score_joueur1, jeu.score_joueur2);
    
    if (jeu.score_joueur1 == jeu.score_joueur2) {
        printf("Match nul!\n");
    } else {
        printf("Le gagnant est %s!\n", (jeu.score_joueur1 > jeu.score_joueur2) ? challenger : challenged);
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
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];
    printf("\n\033[1;36m=== Bio Options ===\033[0m\n");
    printf("1. Set your bio\n");
    printf("2. View someone's bio\n");
    printf("Others. Return to main menu\n");
    printf("Choice: ");

    if (fgets(input, sizeof(input), stdin) != NULL)
    {
        int bio_choice = atoi(input);
        if (bio_choice == 1)
        {
            char bio[MAX_BIO_LENGTH] = "setbio:";
            printf("\033[1;34mEnter your bio:\033[0m\n");
            get_multiline_input(bio + 7, sizeof(bio) - 7);
            write_server(sock, bio);
        }
        else if (bio_choice == 2)
        {
            printf("\033[1;34mEnter nickname to view bio: \033[0m");
            if (fgets(input, sizeof(input), stdin) != NULL)
            {
                input[strcspn(input, "\n")] = '\0'; // Remove newline
                snprintf(buffer, sizeof(buffer), "getbio:%s", input);
                write_server(sock, buffer);
            }
        }
        else
        {
            printf("\033[1;31mInvalid bio option selected.\033[0m\n");
            printf("\033[1;33mReturning to main menu...\033[0m\n");
            display_menu(); 
            return ;
        }
    }
}

/**
 * @brief Handles initiating a game of Awale.
 *
 * @param sock The socket connected to the server.
 */
void handle_play_awale(SOCKET sock)
{
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];

    printf("\033[1;34mEnter the nickname of the player you want to play against: \033[0m");
    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline
        snprintf(input, sizeof(input), "awale:%s", buffer);
        write_server(sock, input);
        printf("Waiting for response...\n");
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
 * @brief Handles messages received from the server.
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the server message.
 */
void handle_server_message(SOCKET sock, char *buffer)
{
    // Clear the current line and move cursor up
    printf("\r\033[K\033[A\033[K");

    printf("le buffer est %s\n", buffer);

    bool should_display_menu = false;

    if (strstr(buffer, "[Private") != NULL)
    {
        process_private_message(buffer);
        should_display_menu = !partie_en_cours;
    }
    /*else if (strstr(buffer, "disconnected") != NULL || strstr(buffer, "joined") != NULL)
    {
        process_system_message(buffer);
        should_display_menu = !partie_en_cours;
    }*/
    else if (strstr(buffer, "declined") != NULL)
    {
        should_display_menu = !partie_en_cours;
    }
    else if (strstr(buffer, "[Challenge") != NULL)
    {
        process_challenge_message(buffer);
    }
    else if (strncmp(buffer, "AWALE:", 6) == 0)
    {
        process_awale_message(sock, buffer + 6);
    }
    else if (strncmp(buffer, "ERROR:", 6) == 0)
    {
        process_error_message(sock, buffer);
    }
    else if (strstr(buffer, "fight") != NULL)
    {
        process_fight_message(sock, buffer);
    }
    else if (strstr(buffer, "over") != NULL)
    {
        process_game_over_message(sock, buffer);
        partie_en_cours = false;
        should_display_menu = true;

        if(strstr(buffer, "score") != NULL)
        {
            saver(sock, buffer);
        }
    }
    else
    {
        printf("\033[1;32m%s\033[0m\n", buffer);
        should_display_menu = !partie_en_cours;
    }

    if (should_display_menu)
    {
        display_menu();
        fflush(stdout);
    }
}

/**
 * @brief saves the game
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the server message.
 */
void saver(SOCKET sock, char * buffer){
    // Send to the server if u want to save the game
    char save_game[BUF_SIZE];
    printf("Do you want to save the game? (yes/no)\n");
    //envoyer le message au serveur
    if (fgets(save_game, sizeof(save_game), stdin) != NULL)
    {
        //forme save:pseudo:yes/no
        save_game[strcspn(save_game, "\n")] = '\0'; // Remove newline
        char save_game_message[BUF_SIZE];
        snprintf(save_game_message, sizeof(save_game_message), "save:%s", save_game);
        printf("save_game_message: %s\n", save_game_message);
        write_server(sock, save_game_message);

        // wait for the response
        char response[BUF_SIZE];
        if (read_server(sock, response) == -1)
        {
            return;
        }

        if (response[0]=='-')
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

        printf("save game: %s\n", save[save_count]);

        save_index = (save_index+1) % MAX_PARTIES;
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
    int first, last;
    if (joueur == 1)
    {
        first = 0;
        last = 5;
    }
    else
    {
        first = 6;
        last = 11;
    }

    printf("\n");
    printf("It's your turn, player %d!\n", joueur);
    printf("Enter a number between %d and %d to play\n", first, last);
    printf("Or type 'mp:pseudo:message' to send a private message\n");

    char input[BUF_SIZE];
    while (1)
    {
        if (fgets(input, sizeof(input), stdin) != NULL)
        {
            input[strcspn(input, "\n")] = '\0'; // Remove newline

            // Vérifier si c'est un message privé
            if (strncmp(input, "mp:", 3) == 0)
            {
                write_server(sock, input);
                printf("Message sent! Now please enter your move (%d-%d):\n", first, last);
                continue;
            }

            // Sinon, traiter comme un coup de jeu
            int move_int = atoi(input);
            if (move_int >= first && move_int <= last)
            {
                char buffer[BUF_SIZE];
                snprintf(buffer, sizeof(buffer), "awale_move:%d", move_int);
                write_server(sock, buffer);
                break;
            }
            else
            {
                printf("\033[1;31mInvalid input! Please enter a number between %d and %d or 'mp:pseudo:message':\033[0m\n", first, last);
            }
        }
    }
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
    char move_input[BUF_SIZE];
    int first, last;
    if (joueur == 1)
    {
        first = 0;
        last = 5;
    }
    else
    {
        first = 6;
        last = 11;
    }

    printf("Please enter a valid move between %d and %d:\n", first, last);
    while (1)
    {
        if (fgets(move_input, sizeof(move_input), stdin) != NULL)
        {
            move_input[strcspn(move_input, "\n")] = '\0'; 
            int move_int = atoi(move_input);
            if (move_int >= first && move_int <= last)
            {
                char buffer[BUF_SIZE];
                snprintf(buffer, sizeof(buffer), "awale_move:%d", move_int);
                write_server(sock, buffer);  
                break;
            }
            else
            {
                printf("Invalid range! Please enter a number between %d and %d:\n", first, last);
            }
        }
    }
}

/**
 * @brief Processes fight/challenge messages from the server.
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the fight message.
 */
void process_fight_message(SOCKET sock, char *buffer)
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
    printf("\033[1;33m%s\033[0m\n", buffer); // Jaune pour les messages de fin de partie
    partie_en_cours = false;
    //display_menu(); // Afficher le menu après la fin de partie
}


/**
 * @brief Processes private messages from the server.
 *
 * @param buffer The buffer containing the private message.
 */
void process_private_message(char *buffer)
{
    printf("\033[1;35m%s\033[0m\n", buffer); // Magenta for private messages
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
    printf("\033[1;33m%s\033[0m\n", buffer); // Yellow for challenge messages
    if (strstr(buffer, "Game started") != NULL)
    {
        partie_en_cours = true;  // Set game state before announcing wait
        printf("\033[1;33mStarting game in 5 seconds...\033[0m\n");
#ifdef WIN32
        Sleep(5000); // Windows Sleep in milliseconds
#else
        sleep(5); // Unix sleep in seconds
#endif
        // Ne pas afficher le menu ici car une partie commence
    }
}

/**
 * @brief Displays the main menu to the user.
 */
void display_menu()
{
    printf("\n\033[1;36m=== Chat Menu ===\033[0m\n");
    printf("1. Send message to all\n");
    printf("2. Send private message\n");
    printf("3. List connected users (+ rank)\n");
    printf("4. Bio options\n");
    printf("5. Play awale vs someone\n");
    printf("6. ALl games in progression\n");
    printf("7. See the save games\n");
    printf("8. Clear screen\n");
    printf("9. Quit\n");
    printf("\n");
    printf("Choice: ");
    fflush(stdout);
}

/**
 * @brief Clears the screen using system-specific commands.
 */
void clear_screen_custom()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/**
 * @brief Main application loop handling connection and communication.
 *
 * @param address Server address to connect to.
 * @param name User's name (not used in the current implementation).
 */
void app(const char *address, int port)
{
    SOCKET sock = init_connection(address, port);
    char buffer[BUF_SIZE];
    fd_set rdfs;

    if (!get_valid_pseudo(sock))
    {
        printf("\033[1;31mError getting valid nickname\033[0m\n");
        exit(EXIT_FAILURE);
    }

    clear_screen_custom();
    display_menu();  // Premier affichage du menu

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            handle_user_input(sock);
            // Ne pas afficher le menu ici, il sera affiché après le traitement de la commande
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            int n = read_server(sock, buffer);
            if (n == 0)
            {
                printf("\033[1;31mServer disconnected!\033[0m\n");
                break;
            }
            handle_server_message(sock, buffer);
        }
    }

    end_connection(sock);
}
