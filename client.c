#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#define BUFFER_SIZE 256
#define MAX_BIO_LENGTH 1000

// Structure pour passer les données au thread de lecture
typedef struct {
    int sockfd;
    volatile int *running;
    pthread_mutex_t *mutex;
} ThreadData;

/*void *read_server_messages(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    char buffer[BUFFER_SIZE];
    
    while (*data->running) {
        memset(buffer, 0, BUFFER_SIZE);
        
        pthread_mutex_lock(data->mutex);
        int n = recv(data->sockfd, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
        pthread_mutex_unlock(data->mutex);
        
        if (n > 0) {
            buffer[n] = '\0';  // Assurez-vous que le buffer est correctement terminé
            
            if (strncmp(buffer, "challenge_request:", 17) == 0) {
                char challenger[BUFFER_SIZE];
                strncpy(challenger, buffer + 17, BUFFER_SIZE - 17);
                challenger[strcspn(challenger, "\n")] = 0;
                printf("\nVous avez reçu un défi de %s!\n", challenger);
                printf("Tapez 'accept' ou 'refuse' pour répondre au défi.\n");
            }
            // Réponses aux défis envoyés
            else if (strncmp(buffer, "challenge_response:", 18) == 0) {
                char response[BUFFER_SIZE];
                strncpy(response, buffer + 18, BUFFER_SIZE - 18);
                response[strcspn(response, "\n")] = 0;
                
                if (strcmp(response, "not_found") == 0) {
                    printf("\nJoueur non trouvé.\n");
                } else if (strcmp(response, "in_game") == 0) {
                    printf("\nLe joueur est déjà en partie.\n");
                } else if (strcmp(response, "already_challenged") == 0) {
                    printf("\nLe joueur a déjà un défi en attente.\n");
                } else if (strcmp(response, "self_challenge") == 0) {
                    printf("\nVous ne pouvez pas vous défier vous-même.\n");
                } else if (strcmp(response, "sent") == 0) {
                    printf("\nDéfi envoyé! En attente de réponse...\n");
                }
            }
            else if (strncmp(buffer, "challenge_accepted:", 18) == 0) {
                char opponent[BUFFER_SIZE];
                strncpy(opponent, buffer + 18, BUFFER_SIZE - 18);
                opponent[strcspn(opponent, "\n")] = 0;
                printf("\nDEBUG: Received acceptance from %s\n", opponent);
                printf("\n%s a accepté votre défi! La partie va commencer.\n", opponent);
                printf("Menu: ");
                fflush(stdout);
            }
            else if (strncmp(buffer, "challenge_refused:", 17) == 0) {
                char opponent[BUFFER_SIZE];
                strncpy(opponent, buffer + 17, BUFFER_SIZE - 17);
                printf("\n%s a refusé votre défi.\n", opponent);
                printf("Menu: ");
                fflush(stdout);
            }
            else if (strncmp(buffer, "liste:", 6) != 0) { // Ignore les réponses de liste
                printf("%s", buffer);
                printf("Menu: ");
                fflush(stdout);
            }
        } else if (n == 0) {
            printf("Déconnecté du serveur.\n");
            *data->running = 0;
            break;
        }
        usleep(100000); // Petit délai pour éviter de surcharger le CPU
    }
    return NULL;
}*/


void print_menu() {
    printf("Menu:\n");
    printf("1. Send a message\n");
    printf("2. List connected clients\n");
    printf("3. Exit\n");
    printf("4. Biography\n");
    printf("5. Awale vs Connected Clients\n");
    printf("\n");
}

int get_valid_pseudo(int sockfd, char *buffer) {
    while (1) {
        printf("Enter your pseudo (max 255 characters): ");
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character

        // Send pseudo to server
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("Write error");
            return 0;
        }

        // Wait for server response
        memset(buffer, 0, BUFFER_SIZE);
        if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
            perror("Read error");
            return 0;
        }

        // Check server response
        if (strcmp(buffer, "connected") == 0) {
            printf("Successfully connected to server!\n");
            return 1;
        } else if (strcmp(buffer, "pseudo_exists") == 0) {
            printf("This pseudo is already taken. Please choose another one.\n");
            continue;
        } else {
            printf("Unexpected server response. Please try again.\n");
            return 0;
        }
    }
}

void handle_challenge(int sockfd, pthread_mutex_t *mutex) {
    char buffer[BUFFER_SIZE];
    printf("Enter the pseudo of the player you want to challenge: ");
    char target_pseudo[BUFFER_SIZE];
    
    if (fgets(target_pseudo, BUFFER_SIZE, stdin)) {
        target_pseudo[strcspn(target_pseudo, "\n")] = 0;
        
        pthread_mutex_lock(mutex);
        memset(buffer, 0, BUFFER_SIZE);
        snprintf(buffer, BUFFER_SIZE, "challenge:%s", target_pseudo);
        write(sockfd, buffer, strlen(buffer));
        pthread_mutex_unlock(mutex);
    }
}

int main(int argc, char** argv) { 
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    volatile int running = 1;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

    // Get valid pseudo
    if (!get_valid_pseudo(sockfd, buffer)) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ThreadData thread_data = {sockfd, &running, &mutex};
    pthread_t read_thread;
    //pthread_create(&read_thread, NULL, read_server_messages, &thread_data);

    while (running) {
        print_menu();
        char input[BUFFER_SIZE];
        printf("Menu: ");
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = 0;

        // Traiter d'abord les commandes accept/refuse
        if (strcmp(input, "accept") == 0) {
            printf("Accepting challenge...\n");
            pthread_mutex_lock(&mutex);
            strcpy(buffer, "accept_challenge:");
            write(sockfd, buffer, strlen(buffer));
            pthread_mutex_unlock(&mutex);
            continue;
        }
        else if (strcmp(input, "refuse") == 0) {
            printf("Refusing challenge...\n");
            pthread_mutex_lock(&mutex);
            strcpy(buffer, "refuse_challenge:");
            write(sockfd, buffer, strlen(buffer));
            pthread_mutex_unlock(&mutex);
            continue;
        }

        // Sinon, traiter comme un choix de menu
        int choice = atoi(input);
        switch (choice) {
            case 1: {
                printf("Message: ");
                memset(buffer, 0, BUFFER_SIZE);
                fgets(buffer, BUFFER_SIZE, stdin);
                pthread_mutex_lock(&mutex);
                write(sockfd, buffer, strlen(buffer));
                pthread_mutex_unlock(&mutex);
                break;
            }
            case 2: {
                pthread_mutex_lock(&mutex);
                strcpy(buffer, "liste:");
                write(sockfd, buffer, strlen(buffer));
                
                // Attendre et lire la réponse directement
                memset(buffer, 0, BUFFER_SIZE);
                int n = read(sockfd, buffer, BUFFER_SIZE);
                pthread_mutex_unlock(&mutex);
                
                if (n > 0) {
                    printf("\nConnected clients:\n%s\n", buffer);
                }
                break;
            }

            case 3:{
                printf("Exiting...\n");
                close(sockfd);
                exit(EXIT_SUCCESS);
            }

            case 4:
            {
                printf("\nBio Management:\n");
                printf("1. Set your bio\n");
                printf("2. View someone's bio\n");
                printf("Choose option: ");
                int bio_choice;
                scanf("%d", &bio_choice);
                getchar(); // Consume newline

                if (bio_choice == 1) {
                    memset(buffer, 0, BUFFER_SIZE);
                    strcpy(buffer, "setbio:");
                    printf("Enter your bio (up to 10 lines, end with an empty line):\n");
                    char bio[MAX_BIO_LENGTH] = {0};
                    char line[100];
                    int line_count = 0;
                    
                    while (line_count < 10) {
                        if (fgets(line, sizeof(line), stdin) == NULL) {
                            break;
                        }
                        if (line[0] == '\n') {
                            break;
                        }
                        strcat(bio, line);
                        line_count++;
                    }
                    
                    strcat(buffer, bio);
                    
                    pthread_mutex_lock(&mutex);
                    if (write(sockfd, buffer, strlen(buffer)) < 0) {
                        perror("Write error");
                        pthread_mutex_unlock(&mutex);
                        close(sockfd);
                        exit(EXIT_FAILURE);
                    }
                    pthread_mutex_unlock(&mutex);
                } 
                else if (bio_choice == 2) {
                    printf("Enter pseudo to view bio: ");
                    char target_pseudo[BUFFER_SIZE];
                    fgets(target_pseudo, BUFFER_SIZE, stdin);
                    target_pseudo[strcspn(target_pseudo, "\n")] = '\0';
                    
                    memset(buffer, 0, BUFFER_SIZE);
                    snprintf(buffer, BUFFER_SIZE, "getbio:%s", target_pseudo);
                    
                    pthread_mutex_lock(&mutex);
                    if (write(sockfd, buffer, strlen(buffer)) < 0) {
                        perror("Write error");
                        pthread_mutex_unlock(&mutex);
                        close(sockfd);
                        exit(EXIT_FAILURE);
                    }
                    
                    // Receive and display bio
                    memset(buffer, 0, BUFFER_SIZE);
                    if (read(sockfd, buffer, BUFFER_SIZE) < 0) {
                        perror("Read error");
                        pthread_mutex_unlock(&mutex);
                        close(sockfd);
                        exit(EXIT_FAILURE);
                    }
                    pthread_mutex_unlock(&mutex);
                    
                    printf("\n%s's bio:\n%s\n", target_pseudo, buffer);
                }   
                break;
            }

            /*case 5: {
                handle_challenge(sockfd, &mutex);
                break;
            }*/
            default:
                printf("Invalid choice\n");
                break;
        }
    }

    //pthread_join(read_thread, NULL);
    pthread_mutex_destroy(&mutex);
    close(sockfd);
    return 0;
}