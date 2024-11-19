#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "../awale.h"
#include "server2.h"
#include <sys/select.h>
#include "utilsServer.h"
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

static void app(void)
{
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    int actual = 0;
    int max = sock;
    Client clients[MAX_CLIENTS];
    fd_set rdfs;

    while (1)
    {
        int i = 0;
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        for (i = 0; i < actual; i++)
        {
            FD_SET(clients[i].sock, &rdfs);
        }

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            break;
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);

            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            if (actual >= MAX_CLIENTS)
            {
                const char *msg = "Server is full, please try again later.\n";
                write_client(csock, msg);
                closesocket(csock);
                continue;
            }

            int read_size = read_client(csock, buffer);
            if (read_size <= 0)
            {
                closesocket(csock);
                continue;
            }

            buffer[read_size] = 0; // Ensure null termination

            // Vérification du pseudo
            if (!check_pseudo(clients, actual, buffer))
            {
                const char *error_msg;
                if (strlen(buffer) < PSEUDO_MIN_LENGTH)
                {
                    error_msg = "pseudo_too_short";
                }
                else if (strlen(buffer) >= PSEUDO_MAX_LENGTH)
                {
                    error_msg = "pseudo_too_long";
                }
                else
                {
                    error_msg = "pseudo_exists";
                }
                write_client(csock, error_msg);
                closesocket(csock);
                continue;
            }

            // Si on arrive ici, le pseudo est valide
            max = csock > max ? csock : max;

            Client c = {csock};
            strncpy(c.name, buffer, PSEUDO_MAX_LENGTH - 1);
            c.name[PSEUDO_MAX_LENGTH - 1] = 0; // Ensure null termination
            c.has_bio = 0;
            clients[actual] = c;
            actual++;

            write_client(csock, "connected");

            // Inform other clients
            char connect_msg[BUF_SIZE];
            snprintf(connect_msg, BUF_SIZE, "%s has joined the chat!\n", buffer);
            send_message_to_all_clients(clients, c, actual, connect_msg, 1);
        }
        else
        {
            int i = 0;
            for (i = 0; i < actual; i++)
            {
                if (FD_ISSET(clients[i].sock, &rdfs))
                {
                    Client client = clients[i];
                    int c = read_client(clients[i].sock, buffer);

                    if (c == 0)
                    {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual);

                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected!\n", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1);
                    }
                    else
                    {
                        send_message_to_all_clients(clients, client, actual, buffer, 1);
                        if (strncmp(buffer, "list:", 5) == 0)
                        {
                            list_connected_clients(clients, actual, i);
                        }
                        else if (strncmp(buffer, "mp:", 3) == 0)
                        {
                            handle_private_message(clients, actual, i, buffer);
                        }
                        else if (strncmp(buffer, "setbio:", 7) == 0 ||
                                 strncmp(buffer, "getbio:", 7) == 0)
                        {
                            handle_bio_command(clients, actual, i, buffer);
                        }
                        else if (strncmp(buffer, "awale:", 6) == 0)
                        {

                            handle_awale_challenge(clients, actual, i, buffer + 6);
                        }
                        else if (strncmp(buffer, "awale_response:", 15) == 0)
                        {
                            handle_awale_response(clients, actual, i, buffer + 15);
                        }
                        else if (strncmp(buffer, "awale_move:", 11) == 0)
                        {
                            handle_awale_move(clients, actual, i, buffer + 11);
                        }
                        else if (strncmp(buffer, "awale_list:", 11) == 0)
                        {
                            handle_awale_list(clients, actual, i);
                        }
                        else if (strncmp(buffer, "privacy:", 8) == 0)
                        {
                            handle_privacy(clients,  actual,  i, buffer +8);
                        }
                        else if (strncmp(buffer, "spectators:", 11) == 0)
                        {
                            printf("handling the spectators");
                            handle_spectators(clients, actual, i, buffer);
                        }
                        else
                        {
                            send_message_to_all_clients(clients, client, actual, buffer, 0);
                        }
                    }
                    break;
                }
            }
        }
    }

    clear_clients(clients, actual);
    end_connection(sock);
}

int main(int argc, char **argv)
{

    init();

    printf("Server started on port %d\n", PORT);
    app();

    end();

    return EXIT_SUCCESS;
}
