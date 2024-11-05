#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256
#define MAX_BIO_LENGTH 1000

typedef struct {
    int socket;
    char pseudo[BUFFER_SIZE];
    char bio[MAX_BIO_LENGTH];
    int has_bio;
} Client;

typedef struct {
    Client clients[MAX_CLIENTS];
    int client_count;
    sem_t semaphore; 
} SharedData;

SharedData *shared_data;
int sockfd;

int pseudo_exists(const char* pseudo) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (shared_data->clients[i].socket != -1 && 
            strcmp(shared_data->clients[i].pseudo, pseudo) == 0) {
            return 1;
        }
    }
    return 0;
}

void handle_client(int client_index) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 50];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = read(shared_data->clients[client_index].socket, buffer, BUFFER_SIZE);
        if (n <= 0) {
            printf("Client disconnected: %s\n", shared_data->clients[client_index].pseudo);
            
            sem_wait(&shared_data->semaphore);
            close(shared_data->clients[client_index].socket);
            shared_data->clients[client_index].socket = -1;
            sem_post(&shared_data->semaphore);
            break;
        }
        
        snprintf(message, sizeof(message), "%s: %s", shared_data->clients[client_index].pseudo, buffer);
        printf("%s\n", message);

        if (strncmp(buffer, "liste:", 6) == 0) {
            char pseudo_list[BUFFER_SIZE * MAX_CLIENTS] = {0};
            
            sem_wait(&shared_data->semaphore);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (shared_data->clients[i].socket != -1) {
                    strncat(pseudo_list, shared_data->clients[i].pseudo, BUFFER_SIZE - strlen(pseudo_list) - 1);
                    strncat(pseudo_list, "\n", BUFFER_SIZE - strlen(pseudo_list) - 1);
                }
            }
            sem_post(&shared_data->semaphore);
            
            // S'assurer que le message est correctement terminé
            strncat(pseudo_list, "\n", BUFFER_SIZE - strlen(pseudo_list) - 1);
            
            // Utiliser send avec MSG_NOSIGNAL pour éviter les problèmes de SIGPIPE
            if (send(shared_data->clients[client_index].socket, pseudo_list, strlen(pseudo_list), MSG_NOSIGNAL) < 0) {
                perror("Send error");
                break;
            }
        }
        else if (strncmp(buffer, "setbio:", 7) == 0) {
            sem_wait(&shared_data->semaphore);
            strncpy(shared_data->clients[client_index].bio, buffer + 7, MAX_BIO_LENGTH - 1);
            shared_data->clients[client_index].has_bio = 1;
            sem_post(&shared_data->semaphore);
            
            printf("Bio updated for %s\n", shared_data->clients[client_index].pseudo);
        }
        else if (strncmp(buffer, "getbio:", 7) == 0) {
            char target_pseudo[BUFFER_SIZE];
            strncpy(target_pseudo, buffer + 7, BUFFER_SIZE - 1);
            
            char response[MAX_BIO_LENGTH] = "Bio not found.";
            
            sem_wait(&shared_data->semaphore);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (shared_data->clients[i].socket != -1 && 
                    strcmp(shared_data->clients[i].pseudo, target_pseudo) == 0 &&
                    shared_data->clients[i].has_bio) {
                    strncpy(response, shared_data->clients[i].bio, MAX_BIO_LENGTH - 1);
                    break;
                }
            }
            sem_post(&shared_data->semaphore);
            
            if (send(shared_data->clients[client_index].socket, response, strlen(response), MSG_NOSIGNAL) < 0) {
                perror("Send error");
                break;
            }
        }
    }
}

void cleanup() {
    sem_wait(&shared_data->semaphore);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (shared_data->clients[i].socket != -1) {
            close(shared_data->clients[i].socket);
        }
    }
    sem_post(&shared_data->semaphore);
    
    sem_destroy(&shared_data->semaphore);
    shmdt(shared_data);
    shmctl(IPC_PRIVATE, IPC_RMID, NULL);
    close(sockfd);
    exit(0);
}

int main(int argc, char** argv) {
    struct sockaddr_in cli_addr, serv_addr;
    socklen_t clilen = sizeof(cli_addr);
    char buffer[BUFFER_SIZE];

    if (argc != 2) {
        printf("usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("Server starting...\n");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    // Enable address reuse
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt error");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    listen(sockfd, MAX_CLIENTS);
    printf("Waiting for clients...\n");

    int shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget error");
        exit(EXIT_FAILURE);
    }

    shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (SharedData *) -1) {
        perror("shmat error");
        exit(EXIT_FAILURE);
    }

    shared_data->client_count = 0;
    sem_init(&shared_data->semaphore, 1, 1);
    
    // Initialiser les sockets à -1
    for (int i = 0; i < MAX_CLIENTS; i++) {
        shared_data->clients[i].socket = -1;
    }

    signal(SIGINT, cleanup);

    while (1) {
        int newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("Accept error");
            continue;
        }

        printf("Connection accepted from %s\n", inet_ntoa(cli_addr.sin_addr));
        
        // Lire le pseudo
        memset(buffer, 0, BUFFER_SIZE);
        if (read(newsockfd, buffer, BUFFER_SIZE) <= 0) {
            close(newsockfd);
            continue;
        }

        // Vérifier si le pseudo existe déjà
        sem_wait(&shared_data->semaphore);
        int pseudo_taken = pseudo_exists(buffer);
        sem_post(&shared_data->semaphore);

        if (pseudo_taken) {
            printf("Pseudo '%s' already exists, rejecting connection\n", buffer);
            const char *error_msg = "pseudo_exists";
            send(newsockfd, error_msg, strlen(error_msg), MSG_NOSIGNAL);
            close(newsockfd);
            continue;
        }

        // Accepter la connexion si le pseudo est unique
        const char *success_msg = "connected";
        send(newsockfd, success_msg, strlen(success_msg), MSG_NOSIGNAL);

        // Trouver un slot libre
        int client_index = -1;
        sem_wait(&shared_data->semaphore);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (shared_data->clients[i].socket == -1) {
                client_index = i;
                shared_data->clients[i].socket = newsockfd;
                strncpy(shared_data->clients[i].pseudo, buffer, BUFFER_SIZE - 1);
                shared_data->clients[i].has_bio = 0;
                shared_data->client_count++;
                break;
            }
        }
        sem_post(&shared_data->semaphore);

        if (client_index == -1) {
            printf("Server full, connection rejected\n");
            close(newsockfd);
            continue;
        }

        printf("Client connected: %s (index: %d)\n", buffer, client_index);

        if (fork() == 0) {
            close(sockfd);  // Fermer le socket d'écoute dans le processus enfant
            handle_client(client_index);
            exit(0);
        }
        
        close(newsockfd);  // Fermer le socket client dans le processus parent
    }

    return 0;
}