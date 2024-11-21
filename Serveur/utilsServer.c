#include "utilsServer.h"

// Global
//pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
AwaleChallenge awale_challenges[MAX_CHALLENGES];
int challenge_count = 0;
PartieAwale awale_parties[MAX_PARTIES];
int partie_count = 0;

#include "server2.h"
#include <errno.h>
#include <sys/select.h>

// Function to initialize the program
void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

// Function to end the program
void end(void)
{
    //pthread_mutex_destroy(&clients_mutex);
    //pthread_mutex_destroy(&socket_mutex);
#ifdef WIN32
    WSACleanup();
#endif
}
int init_connection(int port)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        perror("bind()");
        closesocket(sock);
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        perror("listen()");
        closesocket(sock);
        exit(errno);
    }

    return sock;
}

// Function to end a connection
void end_connection(SOCKET sock)
{
    closesocket(sock);
}

// Function to read from a client
int read_client(SOCKET sock, char *buffer)
{
    int n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n < 0)
    {
        perror("recv()");
        return -1; // Signal an error
    }

    buffer[n] = '\0'; // Null-terminate the buffer
    return n;         // Return number of bytes received
}

// Function to write to a client
void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

// Function to remove a game from the list
void remove_partie(int index, Client *clients)
{
    if (index >= 0 && index < partie_count)
    {

        memmove(&awale_parties[index], &awale_parties[index + 1],
                (partie_count - index - 1) * sizeof(PartieAwale));

        partie_count--;

        for (int i = index; i < partie_count; i++)
        {
            for (int j = 0; j < MAX_CLIENTS; j++)
            {
                if (clients[j].partie_index > index)
                {
                    clients[j].partie_index--;
                }
            }
        }
    }
}

// Function to clear all clients
void clear_clients(Client *clients, int actual)
{
    int i = 0;
    for (i = 0; i < actual; i++)
    {
        closesocket(clients[i].sock);
    }
}

// Function to remove a client from the list
void remove_client(Client *clients, int to_remove, int *actual) {
    if (clients[to_remove].partie_index != -1) {
        PartieAwale *partie = &awale_parties[clients[to_remove].partie_index];
        bool isSpectator = true;
        
        // Check if client is a player or spectator
        if (strcmp(partie->awale_challenge.challenger, clients[to_remove].name) == 0 ||
            strcmp(partie->awale_challenge.challenged, clients[to_remove].name) == 0) {
            isSpectator = false;
        }

        if (isSpectator) {
            // Handle spectator leaving (unchanged)
            remove_spec(partie->Spectators, to_remove, partie->nbSpectators, partie);
        } else {
            // Handle player disconnection - now with Elo update
            const char *challenger = partie->awale_challenge.challenger;
            const char *challenged = partie->awale_challenge.challenged;
            const char *disconnected = clients[to_remove].name;
            const char *winner = strcmp(disconnected, challenger) == 0 ? challenged : challenger;

            // Find the winner client
            Client *winner_client = NULL;
            Client *disconnected_client = &clients[to_remove];

            for (int i = 0; i < *actual; i++) {
                if (strcmp(clients[i].name, winner) == 0) {
                    winner_client = &clients[i];
                    break;
                }
            }

            if (winner_client != NULL) {
                // Update Elo ratings - disconnected player loses
                update_elo_ratings(winner_client, disconnected_client, false);

                char msg[BUF_SIZE];
                snprintf(msg, BUF_SIZE, "\nGame over! %s has disconnected. %s wins by forfeit!\n", 
                        disconnected, winner);
                
                // Notify remaining players and spectators
                for (int i = 0; i < *actual; i++) {
                    if (i != to_remove && clients[i].partie_index == clients[to_remove].partie_index) {
                        write_client(clients[i].sock, msg);
                        clients[i].partie_index = -1;
                    }
                }
            }

            int challenge_index = find_challenge(clients[to_remove].name);
            if (challenge_index != -1) {
                remove_challenge(challenge_index);
            }
            remove_partie(clients[to_remove].partie_index, clients);
        }
    }

    // Move the rest of the clients (unchanged)
    memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
    (*actual)--;
}

// Function to remove a spectator from the list
void remove_spec(Client *clients, int to_remove, int actual, PartieAwale *partie)
{
    if (to_remove >= 0 && to_remove < actual)
    {
        if (partie->nbSpectators > 0)
        {
            memmove(partie->Spectators + to_remove, partie->Spectators + to_remove + 1, (partie->nbSpectators - to_remove - 1) * sizeof(Client));
            partie->nbSpectators--;
        }
    }
}

// Function to send a message to all clients
void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
    int i = 0;
    char message[BUF_SIZE];

    for (i = 0; i < actual; i++)
    {
        /* we don't send message to the sender */
        if (sender.sock != clients[i].sock)
        {
            message[0] = 0; // Reset message buffer for each client

            if (from_server == 0)
            {
                strncpy(message, sender.name, BUF_SIZE - 1);
                strncat(message, " : ", sizeof message - strlen(message) - 1);
                strncat(message, buffer, sizeof message - strlen(message) - 1);
            }
            else
            {
                strncpy(message, buffer, BUF_SIZE - 1);
            }

            write_client(clients[i].sock, message);
        }
    }
}

// Function to add a challenge to the list
Client *findClientByPseudo(Client *clients, int actual, const char *name)
{
    if (clients == NULL || name == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < actual; i++)
    {
        if (!strcmp(name, clients[i].name))
        {
            return &clients[i]; // Return pointer to the matching client
        }
    }

    return NULL;
}

void update_elo_ratings(Client *winner, Client *loser, bool isDraw) {
    int facteur = 32;
    float part_of_calcul_loser = (float)(winner->point - loser->point) / 400;
    float part_of_calcul_winner = (float)(loser->point - winner->point) / 400;
    float p_vict_winner = 1/(1+pow(10, part_of_calcul_winner));
    float p_vict_loser = 1/(1+pow(10, part_of_calcul_loser));

    if (isDraw) {
        winner->point += facteur * (0.5 - p_vict_winner);
        loser->point += facteur * (0.5 - p_vict_loser);
    } else {
        winner->point += facteur * (1 - p_vict_winner);
        loser->point += facteur * (0 - p_vict_loser);
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
        awale_challenges[challenge_count].challenge_time = time(NULL);
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

// function to check if a challenge is expired
void check_challenge_timeouts(Client *clients, int actual) {
    time_t current_time = time(NULL);
    
    for (int i = challenge_count - 1; i >= 0; i--) {
        AwaleChallenge *challenge = &awale_challenges[i];
        Client *challenger = NULL;
        Client *challenged = NULL;

        // Find both players
        for (int j = 0; j < actual; j++) {
            if (strcmp(clients[j].name, challenge->challenger) == 0) {
                challenger = &clients[j];
            }
            if (strcmp(clients[j].name, challenge->challenged) == 0) {
                challenged = &clients[j];
            }
            if (challenger && challenged) break;
        }

        // Check if game is not already started (partie_index == -1)
        if ((challenger && challenger->partie_index == -1) && 
            (challenged && challenged->partie_index == -1) &&
            (current_time - challenge->challenge_time > 60)) {
            
            if (challenger) {
                char msg[BUF_SIZE];
                snprintf(msg, BUF_SIZE, "Challenge to %s has expired (no response within 1 minute)\n", 
                        challenge->challenged);
                write_client(challenger->sock, msg);
            }
            
            if (challenged && strcmp(challenger->name, challenged->name) != 0) {
                char msg[BUF_SIZE];
                snprintf(msg, BUF_SIZE, "Challenge from %s has expired\n",
                        challenge->challenger);
                write_client(challenged->sock, msg);
            }

            remove_challenge(i);
        }
    }
}


/*Client *findClientIndexByPseudo(Client *clients, int actual, const char *name)
{
    if (clients == NULL || name == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < actual; i++)
    {
        if (!strcmp(name, clients[i].name))
        {
            return i; // Return index of the matching client
        }
    }

    return -1;
}*/