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
    CLEAR_SCREEN,
    QUIT
} MenuChoice;

int main(int argc, char **argv)
{
    if (argc < 3)
    { // Ensure both address and name are provided
        printf("Usage: %s [address] [name]\n", argv[0]);
        return EXIT_FAILURE;
    }

    init();
    app(argv[1], argv[2]);
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
    choice = atoi(input);

    switch (choice)
    {
    case SEND_PUBLIC_MESSAGE:
        handle_send_public_message(sock);
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
        handle_privacy(sock);
        handle_allowed_spectators(sock);
        break;

    case LIST_GAMES_IN_PROGRESS:
        handle_list_games(sock);
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
        }
    }
}

/**
 * @brief Handles initiating a game of Awale.
 *
 * @param sock The socket connected to the server.
 */
// void handle_play_awale(SOCKET sock)
// {
//     char buffer[BUF_SIZE];
//     char input[BUF_SIZE];

//     printf("\033[1;34mEnter the nickname of the player you want to play against: \033[0m");
//     if (fgets(buffer, sizeof(buffer), stdin) != NULL)
//     {
//         buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline
//         snprintf(input, sizeof(input), "awale:%s", buffer);
//         write_server(sock, input);
//         printf("Waiting for response...\n");
//     }
// }

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
        printf("Challenge sent to %s. Waiting for response...\n", buffer);

        // After sending the challenge, prompt for privacy settings
        handle_privacy(sock);
    }
}

/**
 * @brief Handles setting the privacy of the game after initiating Awale.
 *
 * @param sock The socket connected to the server.
 */
void handle_privacy(SOCKET sock)
{
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];
    printf("\033[1;34mDo you want to set the game to be private? (yes/no): \033[0m");
    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

        // Validate input
        if (strcasecmp(buffer, "yes") == 0 || strcasecmp(buffer, "no") == 0)
        {
            snprintf(input, sizeof(input), "privacy:%s", buffer);
            write_server(sock, input);
            printf("Privacy setting '%s' sent to server.\n", buffer);

            if (strcasecmp(buffer, "yes") == 0)
            {
                // If private, prompt to set spectators
                handle_allowed_spectators(sock);
                printf("Game set to private.\n");
            }
            else
            {
                // If public, no need to set spectators
                printf("Game set to public. All connected users can be spectators.\n");
            }
        }
        else
        {
            printf("\033[1;31mInvalid input. Please enter 'yes' or 'no'.\033[0m\n");
            handle_privacy(sock); // Retry setting privacy
        }
    }
}

/**
 * @brief Handles setting allowed spectators for a private game.
 *
 * @param sock The socket connected to the server.
 */
void handle_allowed_spectators(SOCKET sock)
{
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];
    printf("\033[1;34mEnter the names of the players you want to add as spectators (separated by commas): \033[0m");
    if (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

        // Validate input: ensure that it's not empty when setting spectators for a private game
        if (strlen(buffer) == 0)
        {
            printf("\033[1;31mNo spectators entered. Please enter at least one name.\033[0m\n");
            handle_allowed_spectators(sock); // Retry setting spectators
            return;
        }

        // Optionally, validate the format of spectator names (e.g., allowed characters)

        snprintf(input, sizeof(input), "spectators:%s", buffer);
        write_server(sock, input);
        printf("Spectator list sent to server.\n");
    }
}

// void handle_allowed_spectators(SOCKET sock)
// {
//     char buffer[BUF_SIZE];
//     char input[BUF_SIZE];
//     printf("\033[1;34mEnter the names of the players you want to add as spectators (separated by commas): \033[0m");
//     if (fgets(buffer, sizeof(buffer), stdin) != NULL)
//     {
//         buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline
//         snprintf(input, sizeof(input), "spectators:%s", buffer);
//         write_server(sock, input);
//     }
// }

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

    // Determine the type of message and display accordingly
    if (strstr(buffer, "[Private") != NULL)
    {
        process_private_message(buffer);
    }
    else if (strstr(buffer, "joined") != NULL || strstr(buffer, "disconnected") != NULL)
    {
        process_system_message(buffer);
    }
    else if (strstr(buffer, "[Challenge") != NULL)
    {
        process_challenge_message(buffer);
    }
    else if (strncmp(buffer, "AWALE:", 6) == 0)
    {
        // Parse and handle Awale game message
        process_awale_message(sock, buffer + 6);
    }
    else if (strncmp(buffer, "ERROR:", 6) == 0)
    {
        // Parse and handle error message
        process_error_message(buffer);
    }
    else if (strstr(buffer, "fight") != NULL)
    {
        // Handle fight/challenge messages
        process_fight_message(sock, buffer);
    }
    else if (strncmp(buffer, "Game over", 9) == 0)
    {
        // Handle game over messages
        process_game_over_message(sock, buffer);
    }
    else
    {
        // Handle normal messages
        printf("\033[1;32m%s\033[0m\n", buffer); // Green for normal messages
    }

    if (partie_en_cours)
    {
        printf("\033[1;33mWaiting for the other player...\033[0m\n");
        // Optionally, disable user input here if needed
    }
    else
    {
        display_menu();
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
    printf("It's your turn, player %d! Please enter a number between %d and %d:\n", joueur, first, last);

    char move_input[BUF_SIZE];
    while (1)
    {
        if (fgets(move_input, sizeof(move_input), stdin) != NULL)
        {
            move_input[strcspn(move_input, "\n")] = '\0'; // Remove newline
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
                printf("Invalid input! Please enter a number between %d and %d:\n", first, last);
            }
        }
    }
}

/**
 * @brief Processes error messages from the server.
 *
 * @param buffer The buffer containing the error message.
 */
void process_error_message(char *buffer)
{
    // Parse the error message
    char *error_type = strtok(buffer, ":");
    char *error_message = strtok(NULL, ":");
    char *joueur_str = strtok(NULL, ":");
    int joueur = (joueur_str != NULL) ? atoi(joueur_str) : 0;

    printf("\033[1;31m%s: %s\033[0m\n", error_type, error_message);

    // Prompt the user to enter a new valid move
    prompt_for_new_move(joueur);
}

/**
 * @brief Prompts the user to enter a new move after an error.
 *
 * @param joueur The player's number.
 */
void prompt_for_new_move(int joueur)
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
            move_input[strcspn(move_input, "\n")] = '\0'; // Remove newline
            int move_int = atoi(move_input);
            if (move_int >= first && move_int <= last)
            {
                char buffer[BUF_SIZE];
                snprintf(buffer, sizeof(buffer), "awale_move:%d", move_int);
                write_server(STDIN_FILENO, buffer); // Assuming STDIN_FILENO is replaced with the actual socket
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
    printf("\033[1;31m%s\033[0m\n", buffer); // Red for game over messages
    partie_en_cours = false;
    write_server(sock, "ack_gameover"); // Send acknowledgment to server
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
        printf("\033[1;33mStarting game in 5 seconds...\033[0m\n");
#ifdef WIN32
        Sleep(5000); // Windows Sleep in milliseconds
#else
        sleep(5); // Unix sleep in seconds
#endif
        // Wait to receive the game board from the server
    }
}

/**
 * @brief Displays the main menu to the user.
 */
void display_menu()
{
    printf("\n\033[1;36m=== Main Menu ===\033[0m\n");
    printf("1. Send public message\n");
    printf("2. Send private message\n");
    printf("3. List users\n");
    printf("4. Bio options\n");
    printf("5. Play Awale\n");
    printf("6. List games in progress\n");
    printf("7. Clear screen\n");
    printf("8. Quit\n");
    printf("Choice: ");
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
void app(const char *address, const char *name)
{
    SOCKET sock = init_connection(address);
    char buffer[BUF_SIZE];
    fd_set rdfs;

    // Get and validate user's nickname
    if (!get_valid_pseudo(sock))
    {
        printf("\033[1;31mError getting valid nickname\033[0m\n");
        exit(EXIT_FAILURE);
    }

    clear_screen_custom();
    display_menu();

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        // Wait for input from user or server
        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            handle_user_input(sock);
            if (!partie_en_cours)
            {
                display_menu();
            }
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
