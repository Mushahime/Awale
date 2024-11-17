#include "utilsServer.h"
#include "server2.h"
#include <errno.h>
#include <sys/select.h>

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

void end(void)
{
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&socket_mutex);
#ifdef WIN32
    WSACleanup();
#endif
}
int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
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

void end_connection(SOCKET sock)
{
    closesocket(sock);
}

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

void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

void remove_partie(int index)
{
    if (index >= 0 && index < partie_count)
    {
        memmove(&awale_parties[index], &awale_parties[index + 1],
                (partie_count - index - 1) * sizeof(PartieAwale));
        partie_count--;
    }
}

void clear_clients(Client *clients, int actual)
{
    int i = 0;
    for (i = 0; i < actual; i++)
    {
        closesocket(clients[i].sock);
    }
}

void remove_client(Client *clients, int to_remove, int *actual)
{
    // Si le client était dans une partie
    if (clients[to_remove].partie_index != -1)
    {
        int partie_index = clients[to_remove].partie_index;

        // Trouver et notifier l'autre joueur
        for (int i = 0; i < *actual; i++)
        {
            if (i != to_remove && clients[i].partie_index == partie_index)
            {
                char msg[BUF_SIZE];
                char buffer[BUF_SIZE];
                snprintf(msg, BUF_SIZE, "Game over! %s has left\n", clients[to_remove].name);
                write_client(clients[i].sock, msg);

                // Attendre l'acquittement avec un select() pour éviter de bloquer indéfiniment
                fd_set rdfs;
                struct timeval tv;
                tv.tv_sec = 5; // timeout de 5 secondes
                tv.tv_usec = 0;

                FD_ZERO(&rdfs);
                FD_SET(clients[i].sock, &rdfs);

                if (select(clients[i].sock + 1, &rdfs, NULL, NULL, &tv) > 0)
                {
                    int n = read_client(clients[i].sock, buffer);
                    if (n > 0 && strncmp(buffer, "ack_gameover", 12) == 0)
                    {
                        // Acquittement reçu, on peut continuer
                        clients[i].partie_index = -1;
                        break;
                    }
                }
                // Même si pas d'acquittement, on continue après le timeout
                clients[i].partie_index = -1;
                break;
            }
        }

        // Supprimer la partie
        remove_partie(partie_index);
    }

    memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
    (*actual)--;
}

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
                // Pour les messages normaux des utilisateurs
                strncpy(message, sender.name, BUF_SIZE - 1);
                strncat(message, " : ", sizeof message - strlen(message) - 1);
                strncat(message, buffer, sizeof message - strlen(message) - 1);
            }
            else
            {
                // Pour les messages système (join, disconnect, etc.)
                strncpy(message, buffer, BUF_SIZE - 1);
            }

            write_client(clients[i].sock, message);
        }
    }
}