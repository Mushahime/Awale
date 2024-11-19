#include "client2.h"
#include "commands.h"
#include "../awale.h"

// Global variables
char pseudo[PSEUDO_MAX_LENGTH];
char save[MAX_PARTIES][BUF_SAVE_SIZE+BUF_SIZE];
int save_count = 0;
int save_index = 0;
bool partie_en_cours = false;


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
 * @brief Handles messages received from the server.
 *
 * @param sock The socket connected to the server.
 * @param buffer The buffer containing the server message.
 */
void handle_server_message(SOCKET sock, char *buffer)
{
    // Clear the current line and move cursor up
    printf("\r\033[K\033[A\033[K");

    //printf("le buffer est %s\n", buffer);

    bool should_display_menu = false;

    if (strstr(buffer, "[Private") != NULL)
    {
        process_private_message(buffer);
        should_display_menu = !partie_en_cours;
    }
    else if (strstr(buffer, "[Public") != NULL)
    {
        printf("\n");
        printf("\033[1;32m%s\033[0m\n", buffer);
        should_display_menu = !partie_en_cours;
    }
    else if (strstr(buffer, "declined") != NULL)
    {
        partie_en_cours = false;
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
    else if (strncmp(buffer, "FAIL:", 5) == 0)
    {
        printf("\033[1;32m%s\033[0m\n", buffer + 5);
        partie_en_cours = false;
        should_display_menu = true;
    }
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
    else if (strstr(buffer, "[Spectator") != NULL)
    {
        // write in yellow the message
        printf("\n");
        printf("\033[1;33m%s\033[0m\n", buffer);
        partie_en_cours = false;
        should_display_menu = true;
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
    else if (strstr(buffer, "spectating")!=NULL)
    {
        partie_en_cours = true;
        printf("\033[1;32m%s\033[0m\n", buffer);
        should_display_menu = !partie_en_cours;
    }
    else
    {
        printf("\n");
        printf("\033[1;32m%s\033[0m", buffer);
        should_display_menu = !partie_en_cours;
    }

    if (should_display_menu)
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

    if (partie_en_cours)
    {
        if (strncmp(input, "quit", 4) == 0)
        {
            handle_quit_game(sock);
            return;
        }
        else
        if (strncmp(input, "mp:", 3) == 0)
        {
            input[strcspn(input, "\n")] = '\0';
            write_server(sock, input);
        }
        else if (strncmp(input, "\n", 1) != 0) // Ignore les lignes vides
        {
            printf("During a game, you can only send private messages using 'mp:pseudo:message' or 'quit'\n");
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

    case SPEC:
        handle_spec(sock);
        break;

    case CLEAR_SCREEN:
        clear_screen_custom();
        break;

    case QUIT:
        handle_quit();
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
    printf("9.  Clear screen\n");
    printf("10. Quit\n");
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

    if (!get_valid_pseudo(sock))
    {
        printf("\033[1;31mError getting valid nickname\033[0m\n");
        exit(EXIT_FAILURE);
    }

    clear_screen_custom2();
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
