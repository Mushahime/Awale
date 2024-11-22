#include "client2.h"
#include "commands.h"
#include <ctype.h>
#include "../awale.h"

// Global
char pseudo[PSEUDO_MAX_LENGTH];
char save[MAX_PARTIES][BUF_SAVE_SIZE + BUF_SIZE]; // if could have been in serverside
int save_count = 0;
int save_index = 0;
bool partie_en_cours = false;
bool waiting_for_response = false;

// Main function
int main(int argc, char **argv)
{
    if (argc != 4 || atoi(argv[2]) < 1024 || atoi(argv[2]) > 65535)
    {
        printf("Error: Usage: %s <address> <port> <pseudo>\n", argv[0]);
        printf("Port must be between 1024 and 65535\n");
        return EXIT_FAILURE;
    }

    if (strlen(argv[3]) < PSEUDO_MIN_LENGTH || strlen(argv[3]) >= PSEUDO_MAX_LENGTH)
    {
        printf("Error: Pseudo must be between %d and %d characters\n",
               PSEUDO_MIN_LENGTH, PSEUDO_MAX_LENGTH - 1);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < strlen(argv[3]); i++)
    {
        if (!isalnum(argv[3][i]) && argv[3][i] != '_')
        {
            printf("Error: Pseudo must contain only letters, numbers and underscore\n");
            return EXIT_FAILURE;
        }
    }

    strncpy(pseudo, argv[3], PSEUDO_MAX_LENGTH - 1);
    pseudo[PSEUDO_MAX_LENGTH - 1] = '\0';

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
        printf("\033[1;32mEnter your nickname (2-%d characters, letters, numbers, underscore only): \033[0m",
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
 * @brief Handles messages received from the server.
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the server message.
 */
void handle_server_message(SOCKET sock, char *buffer)
{
    // Clear the current line and move cursor up
    printf("\r\033[K\033[A\033[K");

    bool should_display_menu = false;

    // for private message
    if (strstr(buffer, "[Private") != NULL)
    {
        process_private_message(buffer);
        should_display_menu = !partie_en_cours;
    }
    // for public message
    else if (strstr(buffer, "[Public") != NULL)
    {
        printf("\n");
        printf("\033[1;32m%s\033[0m\n", buffer);
        should_display_menu = !partie_en_cours;
    }
    // for game declined
    else if (strstr(buffer, "declined") != NULL)
    {
        process_challenge_message(buffer);
        partie_en_cours = false;
        should_display_menu = !partie_en_cours;
    }
    // for game challenge response (when not declined)
    else if (strstr(buffer, "[Challenge") != NULL)
    {
        process_challenge_message(buffer);
    }
    // for awale move
    else if (strncmp(buffer, "AWALE:", 6) == 0)
    {
        waiting_for_response = false;
        process_awale_message(sock, buffer + 6);
    }
    // for error message during the game
    else if (strncmp(buffer, "ERROR:", 6) == 0)
    {
        process_error_message(sock, buffer);
    }
    // for fail message for spectator
    else if (strncmp(buffer, "FAIL:", 5) == 0)
    {
        printf("\033[1;32m%s\033[0m\n", buffer + 5);
        partie_en_cours = false;
        should_display_menu = true;
    }
    // for send challenge
    else if (strstr(buffer, "fight") != NULL)
    {
        printf("\n");
        bool test = process_fight_message(sock, buffer);
        if (test)
        {
            partie_en_cours = true;
            should_display_menu = !partie_en_cours;
        }
        else
        {
            partie_en_cours = false;
            should_display_menu = !partie_en_cours;
        }

        printf("\n");
    }
    // for friends list
    else if (strstr(buffer, "[Friend]") != NULL)
    {
        printf("\n");
        process_friend_message(sock, buffer);
    }
    // for spectator when the game is over
    else if (strstr(buffer, "[Spectator") != NULL)
    {
        printf("\n");
        printf("\033[1;33m%s\033[0m\n", buffer);
        partie_en_cours = false;
        should_display_menu = true;
    }
    // for game over message (not for spectator)
    else if (strstr(buffer, "over") != NULL)
    {
        process_game_over_message(sock, buffer);
        partie_en_cours = false;
        should_display_menu = true;

        if (strstr(buffer, "score") != NULL)
        {
            saver(sock, buffer);
        }
    }
    // for expired game
    else if (strstr(buffer, "expired") != NULL)
    {
        printf("\033[1;31m%s\033[0m\n", buffer);
        partie_en_cours = false;
        should_display_menu = true;
        waiting_for_response = false;
    }
    // for spectator when he is accepted in a game
    else if (strstr(buffer, "spectating") != NULL)
    {
        partie_en_cours = true;
        printf("\033[1;32m%s\033[0m\n", buffer);
        should_display_menu = !partie_en_cours;
    }
    // rest
    else
    {
        if (strstr(buffer, "already in a challenge") != NULL || 
            strstr(buffer, "not found") != NULL || (strstr(buffer, "Invalid") != NULL))
        {
            waiting_for_response = false;
        }
        printf("\n");
        printf("\033[1;32m%s\033[0m", buffer);
        should_display_menu = !partie_en_cours;
    }

    if (should_display_menu && !waiting_for_response)
    {
        display_menu();
        fflush(stdout);
    }
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

    // Handle game input
    if (partie_en_cours)
    {
        if (strncmp(input, "quit", 4) == 0)
        {
            handle_quit_game(sock);
            return;
        }
        else if (strncmp(input, "mp:", 3) == 0)
        {
            input[strcspn(input, "\n")] = '\0';
            write_server(sock, input);
        }
        else if (strncmp(input, "\n", 1) != 0)
        {
            printf("During a game, you can only send private messages using 'mp:pseudo:message' or 'quit'\n");
        }
        return;
    }

    if (waiting_for_response)
    {
        // printf("\033[1;31mYou are waiting for a response. Please wait.\033[0m\n");
        return;
    }
    choice = atoi(input);

    // Handle user choice (see in game)
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

    case SPEC:
        handle_spec(sock);
        break;

    case BLOCK:
        handle_block(sock);
        break;

    case FRIEND:
        handle_friend(sock);
        break;

    case CLEAR_SCREEN:
        clear_screen_custom();
        break;

    case QUIT:
        handle_quit();
        break;

    case NOT_IMPLEMENTED:
        printf("\033[1;31mNot implemented yet.\033[0m\n");
        printf("\033[1;31m- Tournament\033[0m\n");
        printf("\033[1;31m- Save game -> Persistance\033[0m\n");
        printf("\033[1;31m- Secure connection\033[0m\n");
        printf("\033[1;31m- Matchmaking\033[0m\n");
        printf("\033[1;31m- AI for games\033[0m\n");
        display_menu();
        break;

    default:
        printf("\033[1;31mInvalid choice.\033[0m\n");
        display_menu();
        break;
    }
}

/**
 * @brief Displays the main menu to the user.
 */
void display_menu()
{
    printf("\n\033[1;36m=== Chat Menu ===\033[0m\n");
    printf("1.  Send message to all\n");
    printf("2.  Send private message\n");
    printf("3.  List connected users (+ rank)\n");
    printf("4.  Bio options\n");
    printf("5.  Play awale vs someone\n");
    printf("6.  ALl games in progression\n");
    printf("7.  See the save games\n");
    printf("8.  Spectate a game\n");
    printf("9.  Blocked users\n");
    printf("10. Friend list\n");
    printf("11. Clear screen\n");
    printf("12. Not Implemented\n");
    printf("13. Quit\n");
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
    display_menu();
}

void clear_screen_custom2()
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

    write_server(sock, pseudo);

    /*printf("Sent pseudo1: %s\n", pseudo);
    printf("Server response1: %s\n", buffer);*/

    // wait for the server response
    if (read_server(sock, buffer) == -1 || strcmp(buffer, "connected") != 0)
    {
        if (strcmp(buffer, "pseudo_exists") == 0)
        {
            printf("\033[1;31mThis nickname is already taken.\033[0m\n");
        }
        else
        {
            printf("\033[1;31mConnection failed. Invalid nickname or server error.\033[0m\n");
        }
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Sent pseudo: %s\n", pseudo);
    printf("Server response: %s\n", buffer);

    printf("\033[1;32mConnected successfully!\033[0m\n");
    clear_screen_custom2();
    display_menu();

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        // if there is an input from the user or the server
        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        // if there is an input from the user
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            handle_user_input(sock);
        }
        // if there is an input from the server
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
