// client.c
#include "client2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // For close()
#include <arpa/inet.h> // For inet_ntoa()
#include <netdb.h>     // For gethostbyname()
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/select.h>

void init_client(void)
{
    // No initialization needed for POSIX
}

void close_client(void)
{
    // No cleanup needed for POSIX
}

void run_client(const char *address, const char *name)
{
    int sock = init_connection(address);
    char buffer[BUF_SIZE];
    fd_set rdfs;

    // Send our name to the server
    write_to_server(sock, name);

    printf("Welcome to the Awale Game!\n");
    printf("Commands:\n");
    printf("  /play        - Invite another player to start a game\n");
    printf("  /players     - List online players\n");
    printf("  /exit        - Exit the game\n\n");
    printf("  /message     - Send a message to another player\n");

    while (1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        int maxfd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

        if (select(maxfd + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        // Input from keyboard
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL)
            {
                printf("\nDisconnected from server.\n");
                break;
            }

            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

            if (strcmp(buffer, "/play") == 0)
            {
                printf("Enter the name of the player you want to challenge: ");
                if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL)
                {
                    printf("\nError reading opponent name.\n");
                    continue;
                }
                buffer[strcspn(buffer, "\n")] = '\0';
                char message[BUF_SIZE];
                snprintf(message, BUF_SIZE, "INVITE %s", buffer);
                write_to_server(sock, message);
                printf("Game invitation sent to %s.\n", buffer);
            }
            else if (strcmp(buffer, "/players") == 0)
            {
                write_to_server(sock, "LIST");
            }
            else if (strcmp(buffer, "/exit") == 0)
            {
                printf("Exiting the game. Goodbye!\n");
                break;
            }
            else if (strcmp(buffer, "/message") == 0)
            {
                printf("Enter the name of the player you want to send a message to: ");
                if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL)
                {
                    printf("\nError reading opponent name.\n");
                    continue;
                }

                // Remove the newline character from the end of the receiver's name
                int receiver_name_len = strcspn(buffer, "\n");
                buffer[receiver_name_len] = '\0';

                // Store the receiver's name
                char receiver_name[BUF_SIZE];
                strncpy(receiver_name, buffer, receiver_name_len);
                receiver_name[receiver_name_len] = '\0'; // Ensure null termination

                // Prompt for the message to send
                printf("Enter the message to send: ");
                if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL)
                {
                    printf("\nError reading message.\n");
                    continue;
                }

                // Remove newline from the message
                int message_len = strcspn(buffer, "\n");
                buffer[message_len] = '\0';

                // Construct the final message to send to the server
                char final_message[BUF_SIZE];
                snprintf(final_message, BUF_SIZE, "MESSAGE%2d%s%s", receiver_name_len, receiver_name, buffer);

                // Send the message to the server
                write_to_server(sock, final_message);
                printf("Message sent to %s.\n", receiver_name);

            }
            else
            {
                // Send message to server
                write_to_server(sock, buffer);
            }
        }

        // Data from server
        if (FD_ISSET(sock, &rdfs))
        {
            int n = read_from_server(sock, buffer);
            if (n == 0)
            {
                printf("Server disconnected!\n");
                break;
            }
            printf("%s\n", buffer);

            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline

            if (strncmp(buffer, "INVITE_FROM ", 12) == 0)
            {
                const char *inviter_name = buffer + 12;
                printf("%s has invited you to play. Accept? (y/n): ", inviter_name);
                char response[10];
                if (fgets(response, sizeof(response), stdin))
                {
                    response[strcspn(response, "\n")] = '\0';
                    if (strcmp(response, "y") == 0 || strcmp(response, "Y") == 0)
                    {
                        write_to_server(sock, "ACCEPT");
                        printf("You accepted the invitation. The game is starting!\n");
                        // Implement game logic here
                    }
                    else
                    {
                        write_to_server(sock, "REFUSE");
                        printf("You declined the invitation.\n");
                    }
                }
            }
            else if (strcmp(buffer, "ACCEPTED") == 0)
            {
                printf("Your game invitation was accepted. The game is starting!\n");
                // Implement game logic here
            }
            else if (strcmp(buffer, "REFUSED") == 0)
            {
                printf("Your game invitation was declined.\n");
            }
            else if (strcmp(buffer, "Your opponent has disconnected.") == 0)
            {
                printf("%s\n", buffer);
            }
            // else
            // {
            //     // Print message
            // }
            // printf("%s\n", buffer);
        }
    }

    close_connection(sock);
}

int init_connection(const char *address)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in sin = {0};

    if (sock == -1)
    {
        perror("socket()");
        exit(errno);
    }

    struct hostent *hostinfo = gethostbyname(address);
    if (hostinfo == NULL)
    {
        fprintf(stderr, "Unknown host %s.\n", address);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(struct in_addr *)hostinfo->h_addr;
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
        perror("connect()");
        exit(errno);
    }

    return sock;
}

void close_connection(int sock)
{
    close(sock);
}

int read_from_server(int sock, char *buffer)
{
    int n = recv(sock, buffer, BUF_SIZE - 1, 0);
    if (n <= 0)
    {
        return 0;
    }
    buffer[n] = '\0';
    return n;
}

void write_to_server(int sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) == -1)
    {
        perror("send()");
        exit(errno);
    }
}



int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: %s [address] [name]\n", argv[0]);
        return EXIT_FAILURE;
    }

    init_client();
    run_client(argv[1], argv[2]);
    close_client();

    return EXIT_SUCCESS;
}
