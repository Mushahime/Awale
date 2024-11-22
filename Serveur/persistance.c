#include "persistance.h"
#include <stdio.h>

// These functions are not used in the current implementation. But they can be used to save and load client data for next implementations.

void save_client_data(Client *clients, int actual) {
    FILE *file = fopen(DATA_FILE, "w");
    if (!file) return;
    
    fprintf(file, "%d\n", actual);
    for (int i = 0; i < actual; i++) {
        fprintf(file, "%s\n%d\n%d\n", 
            clients[i].name, 
            clients[i].point,
            clients[i].has_bio);
            
        if (clients[i].has_bio) {
            fprintf(file, "%s\n", clients[i].bio);
        }
        
        // Save friends
        fprintf(file, "%d\n", clients[i].nbFriend);
        for (int j = 0; j < clients[i].nbFriend; j++) {
            fprintf(file, "%s\n", clients[i].friend[j]);
        }
        
        // Save blocked users
        fprintf(file, "%d\n", clients[i].nbBlock);
        for (int j = 0; j < clients[i].nbBlock; j++) {
            fprintf(file, "%s\n", clients[i].block[j]);
        }
    }
    fclose(file);
}

void load_client_data(Client *clients, int *actual) {
    FILE *file = fopen(DATA_FILE, "r");
    if (!file) return;
    
    fscanf(file, "%d\n", actual);
    for (int i = 0; i < *actual; i++) {
        clients[i].sock = INVALID_SOCKET;
        clients[i].partie_index = -1;
        
        fscanf(file, "%s\n", clients[i].name);
        fscanf(file, "%d\n", &clients[i].point);
        fscanf(file, "%d\n", &clients[i].has_bio);
        
        if (clients[i].has_bio) {
            fgets(clients[i].bio, MAX_BIO_LENGTH, file);
            clients[i].bio[strcspn(clients[i].bio, "\n")] = 0;
        }
        
        // Load friends
        fscanf(file, "%d\n", &clients[i].nbFriend);
        for (int j = 0; j < clients[i].nbFriend; j++) {
            fscanf(file, "%s\n", clients[i].friend[j]);
        }
        
        // Load blocked users
        fscanf(file, "%d\n", &clients[i].nbBlock);
        for (int j = 0; j < clients[i].nbBlock; j++) {
            fscanf(file, "%s\n", clients[i].block[j]);
        }
    }
    fclose(file);
}