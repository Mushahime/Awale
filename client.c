#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 256

void print_menu() {
    printf("Menu:\n");
    printf("1. Send a message\n");
    printf("2. List connected clients\n");
    printf("3. Exit\n");
}

int main(int argc, char** argv) { 
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    if (argc != 3) {
        printf("usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Client starting...\n");

    // Initialize server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Prompt for pseudo
    printf("Enter your pseudo (max 255 characters): ");
    memset(buffer, 0, BUFFER_SIZE);
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character

    // Send pseudo to server
    write(sockfd, buffer, strlen(buffer));

    // Message loop
    while (1) {
        print_menu();
        int choice;
        printf("Choose an option: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            // Clear the input buffer
            while (getchar() != '\n');
            continue;
        }
        getchar(); // Consume newline left by scanf
        switch (choice) {
            case 1:
                memset(buffer, 0, BUFFER_SIZE);
                printf("Message: ");
                fgets(buffer, BUFFER_SIZE, stdin);

                // Send message to the server
                if (write(sockfd, buffer, strlen(buffer)) < 0) {
                    perror("Write error");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }
                break;

            case 2:
                memset(buffer, 0, BUFFER_SIZE);
                strcpy(buffer, "liste");

                // Send list command to the server
                if (write(sockfd, buffer, strlen(buffer)) < 0) {
                    perror("Write error");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }

                // Read and print the list of connected clients
                memset(buffer, 0, BUFFER_SIZE);
                printf("Connected clients:\n");
                if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
                    perror("Read error");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }

                printf("%s", buffer);
                
                break;
            case 3:
                printf("Exiting...\n");
                close(sockfd);
                exit(EXIT_SUCCESS);
            default:
                printf("Invalid choice\n");
                continue;
        }

    }

    // Close the socket
    close(sockfd);
    return 0;
}
