#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "../awale.h"
#include "server2.h"
#include <sys/select.h>
#include "utilsServer.h"

// Function to initialize the program
void app(int port)
{
    SOCKET sock = init_connection(port);
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

        //for challenge timeout
        struct timeval timeout;
        timeout.tv_sec = 1;  // Check every second
        timeout.tv_usec = 0;
        
        // Management (read/write) of the sockets
        if (select(max + 1, &rdfs, NULL, NULL, &timeout) == -1)
        {
            perror("select()");
            exit(errno);
        }

        // chalenge timeout
        check_challenge_timeouts(clients, actual);

        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            char temp[BUF_SIZE];
            fgets(temp, sizeof(temp), stdin); // Read and discard stdin input
            continue;
        }
        // New client
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

            // pseudo check
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
                    error_msg = "pseudo_exists or invalid character/name";
                }
                write_client(csock, error_msg);
                closesocket(csock);
                continue;
            }

            // now, valid pseudo
            max = csock > max ? csock : max;

            Client c = {csock};
            strncpy(c.name, buffer, PSEUDO_MAX_LENGTH - 1);
            c.name[PSEUDO_MAX_LENGTH - 1] = 0; // Ensure null termination
            c.has_bio = 0;
            clients[actual] = c;
            clients[actual].partie_index = -1;
            clients[actual].point = 500;
            clients[actual].nbBlock = 0;
            actual++;

            write_client(csock, "connected");

            // Inform other clients
            char connect_msg[BUF_SIZE];
            snprintf(connect_msg, BUF_SIZE, "%s has joined the chat!\n", buffer);
            send_message_to_all_clients(clients, c, actual, connect_msg, 1);
        }
        // New message from a client
        else
        {
            int i = 0;
            for (i = 0; i < actual; i++)
            {
                if (FD_ISSET(clients[i].sock, &rdfs))
                {
                    Client client = clients[i];
                    int c = read_client(clients[i].sock, buffer);

                    // Client disconnected
                    if (c == 0)
                    {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual);
                        sleep(0.5);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected!\n", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1);
                    }
                    // Others cases
                    else
                    {   
                        // for listing all connected clients
                        if (strncmp(buffer, "list:", 5) == 0)
                        {
                            list_connected_clients(clients, actual, i);
                        }
                        // for private message
                        else if (strncmp(buffer, "mp:", 3) == 0)
                        {
                            handle_private_message(clients, actual, i, buffer);
                        }
                        // for bio
                        else if (strncmp(buffer, "setbio:", 7) == 0 ||
                                 strncmp(buffer, "getbio:", 7) == 0)
                        {
                            handle_bio_command(clients, actual, i, buffer);
                        }
                        // for awale challenge
                        else if (strncmp(buffer, "awale:", 6) == 0)
                        {
                            handle_awale_challenge(clients, actual, i, buffer + 6);
                        }
                        // for awale response
                        else if (strncmp(buffer, "awale_response:", 15) == 0)
                        {
                            handle_awale_response(clients, actual, i, buffer + 15);
                        }
                        // for playing awale
                        else if (strncmp(buffer, "awale_move:", 11) == 0)
                        {
                            handle_awale_move(clients, actual, i, buffer + 11);
                        }
                        // for listing all games in progress
                        else if (strncmp(buffer, "awale_list:", 11) == 0)
                        {
                            handle_awale_list(clients, actual, i);
                        }
                        // for saving the game
                        else if (strncmp(buffer, "save:", 5) == 0)
                        {
                            handle_save(clients, actual, i, buffer + 5);
                        }
                        // for spectator (try to spectate a game)
                        else if (strncmp(buffer, "spec:", 5) == 0)
                        {
                            handle_spec(clients, actual, i, buffer + 5);
                        }
                        // for listing blocked users
                        else if (strncmp(buffer, "list_blocked:", 13) == 0)
                        {
                            handle_list_blocked(clients, actual, i);
                        }
                        // for blocking a user
                        else if (strncmp(buffer, "friend:", 7) == 0)
                        {
                            handle_friend(clients, actual, i, buffer + 6);
                        }
                        // for unblocking a user
                        else if (strncmp(buffer, "unfriend:", 9) == 0)
                        {
                            handle_unfriend(clients, actual, i, buffer + 8);
                        }
                                                // for listing blocked users
                        else if (strncmp(buffer, "list_friend:", 11) == 0)
                        {
                            handle_list_friend(clients, actual, i);
                        }
                        // for blocking a user
                        else if (strncmp(buffer, "block:", 6) == 0)
                        {
                            handle_block(clients, actual, i, buffer + 6);
                        }
                        // for unblocking a user
                        else if (strncmp(buffer, "unblock:", 8) == 0)
                        {
                            handle_unblock(clients, actual, i, buffer + 8);
                        }
                        // for quitting the game (spectator or player)
                        else if (strncmp(buffer, "quit_game:", 10) == 0)
                        {
                            handle_quit_game(clients, actual, i);
                        }
                        // friend request respons (form friend_response)
                        else if (strncmp(buffer, "friend_response:", 15) == 0)
                        {
                            handle_friend_response(clients, actual, i, buffer + 15);
                        }
                        else
                        {
                            // add [Public] to the message
                            char public_msg[BUF_SIZE];
                            snprintf(public_msg, BUF_SIZE, "[Public] %s", buffer);
                            send_message_to_all_clients(clients, client, actual, public_msg, 0);
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

// Main function
int main(int argc, char **argv)
{

    init();
    if (argc > 2 || argc < 2 || atoi(argv[1]) < 1024 || atoi(argv[1]) > 65535)
    {
        printf("Error: arguments error or port < 1024 or port > 65535\n");
        return EXIT_FAILURE;
    }

    printf("Server started on port %s\n", argv[1]);
    app(atoi(argv[1]));

    end();

    return EXIT_SUCCESS;
}
