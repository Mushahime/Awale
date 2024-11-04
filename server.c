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

typedef struct {
    int socket;
    char pseudo[BUFFER_SIZE];
} Client;

typedef struct {
    Client clients[MAX_CLIENTS]; // Liste des clients
    int client_count; // Compteur de clients
    sem_t semaphore; // Sémaphore pour la synchronisation
} SharedData;

SharedData *shared_data; // Pointeur vers la mémoire partagée
int sockfd; // Déclaré globalement pour qu'il soit accessible dans le gestionnaire de signal

// Fonction pour gérer la communication avec un client
void handle_client(int client_index) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 50];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = read(shared_data->clients[client_index].socket, buffer, BUFFER_SIZE);
        if (n <= 0) {
            printf("Client disconnected: %s\n", shared_data->clients[client_index].pseudo);
            close(shared_data->clients[client_index].socket);
            sem_wait(&shared_data->semaphore); // Prendre le sémaphore
            shared_data->clients[client_index].socket = -1; // Marquer le client comme déconnecté
            sem_post(&shared_data->semaphore); // Libérer le sémaphore
            break;
        }
        
        // Afficher le message avec le pseudo du client
        snprintf(message, sizeof(message), "%s: %s", shared_data->clients[client_index].pseudo, buffer);
        printf("%s", message);

        // Vérifier si la commande est "liste"
        if (strncmp(buffer, "liste", 5) == 0) {
            char pseudo_list[BUFFER_SIZE * MAX_CLIENTS] = {0}; // Initialiser à vide

            sem_wait(&shared_data->semaphore); // Prendre le sémaphore
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (shared_data->clients[i].socket != -1) {
                    // Ajouter le pseudo et une nouvelle ligne à la liste
                    strncat(pseudo_list, shared_data->clients[i].pseudo, BUFFER_SIZE - strlen(pseudo_list) - 1);
                    strncat(pseudo_list, " ", BUFFER_SIZE - strlen(pseudo_list) - 1); // Ajouter une nouvelle ligne pour la lisibilité
                }
            }
            sem_post(&shared_data->semaphore); // Libérer le sémaphore

            strncat(pseudo_list, "\n\n", BUFFER_SIZE - strlen(pseudo_list) - 1); // Ajouter une nouvelle ligne à la fin

            printf("Connected clients:\n%s", pseudo_list);
            write(shared_data->clients[client_index].socket, pseudo_list, strlen(pseudo_list)); // Envoyer la liste au client
        }
    }
}

// Fonction de nettoyage pour fermer tous les sockets
void cleanup() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (shared_data->clients[i].socket != -1) {
            close(shared_data->clients[i].socket);
        }
    }
    // Détacher et supprimer la mémoire partagée
    shmdt(shared_data);
    shmctl(IPC_PRIVATE, IPC_RMID, NULL); // Supprimer le segment de mémoire partagée
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

    // Crée le socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        exit(EXIT_FAILURE);
    }

    // Initialise l'adresse du serveur
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    // Lie le socket
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Écoute les connexions entrantes
    listen(sockfd, MAX_CLIENTS);
    printf("Waiting for clients...\n");

    // Créer un segment de mémoire partagée
    int shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("shmget error");
        exit(EXIT_FAILURE);
    }

    // Attacher le segment de mémoire partagée
    shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (SharedData *) -1) {
        perror("shmat error");
        exit(EXIT_FAILURE);
    }

    // Initialiser le compteur de clients et le sémaphore
    shared_data->client_count = 0;
    sem_init(&shared_data->semaphore, 1, 1); // 1 est pour le processus, 1 signifie que le sémaphore est disponible

    // Configurer le gestionnaire de signal
    signal(SIGINT, cleanup);

    while (shared_data->client_count < MAX_CLIENTS) {
        int newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("Accept error");
            continue;
        }

        // Enregistre le nouveau client
        printf("Connection accepted from %s\n", inet_ntoa(cli_addr.sin_addr));
        memset(buffer, 0, BUFFER_SIZE);
        read(newsockfd, buffer, BUFFER_SIZE);

        // Créer un nouveau processus pour gérer la communication avec le client
        if (fork() == 0) {
            // Processus enfant : gérer la communication avec le client
            handle_client(shared_data->client_count - 1); // Passer l'index correct du client
            close(newsockfd);
            exit(0);
        }

        sem_wait(&shared_data->semaphore); // Prendre le sémaphore
        // Store the new client's socket and pseudo
        shared_data->clients[shared_data->client_count].socket = newsockfd;
        strncpy(shared_data->clients[shared_data->client_count].pseudo, buffer, BUFFER_SIZE - 1);
        shared_data->clients[shared_data->client_count].pseudo[BUFFER_SIZE - 1] = '\0'; // Assurer la terminaison nulle

        printf("Client pseudo: %s\n", shared_data->clients[shared_data->client_count].pseudo);
        
        // Incrémenter le compteur de clients et libérer le sémaphore après modification
        shared_data->client_count++;
        sem_post(&shared_data->semaphore); // Libérer le sémaphore
    }

    // Fermer le socket du serveur
    close(sockfd);
    return 0;
}
