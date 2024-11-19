#include "utilsServer.h"

// Global
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;
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
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&socket_mutex);
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
            // Manage spectator leaving
            remove_spec(partie->Spectators, to_remove, partie->nbSpectators, partie);
        } else {
            // Manage player leaving
            int challenge_index = find_challenge(clients[to_remove].name);
            if (challenge_index != -1) {
                const char *challenger = awale_challenges[challenge_index].challenger;
                const char *challenged = awale_challenges[challenge_index].challenged;
                const char *other_player = strcmp(clients[to_remove].name, challenger) == 0 ? challenged : challenger;
                
                for (int i = 0; i < *actual; i++) {
                    if (strcmp(clients[i].name, other_player) == 0) {
                        char msg[BUF_SIZE];
                        snprintf(msg, BUF_SIZE, "\033[1;31mChallenge stopped : %s deconnected\033[0m\n",
                                clients[to_remove].name);
                        write_client(clients[i].sock, msg);
                        break;
                    }
                }
                remove_challenge(challenge_index);
            }

            // Notify all players and spectators about game end
            int partie_index = clients[to_remove].partie_index;
            char msg[BUF_SIZE];
            snprintf(msg, BUF_SIZE, "\033[1;31mGame over! %s has left the game\033[0m\n",
                    clients[to_remove].name);
            
            for (int i = 0; i < *actual; i++) {
                if (i != to_remove && clients[i].partie_index == partie_index) {
                    write_client(clients[i].sock, msg);
                    clients[i].partie_index = -1;
                }
            }
            remove_partie(partie_index, clients);
        }
    }

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