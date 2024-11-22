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
        
        fprintf(file, "%d\n", clients[i].nbFriend);
        for (int j = 0; j < clients[i].nbFriend; j++) {
            fprintf(file, "%s\n", clients[i].friend[j]);
        }
        
        fprintf(file, "%d\n", clients[i].nbBlock);
        for (int j = 0; j < clients[i].nbBlock; j++) {
            fprintf(file, "%s\n", clients[i].block[j]);
        }
    }
    fclose(file);
}

void load_client_data(Client *clients, int *actual) {
    FILE *file = fopen(DATA_FILE, "r");
    if (!file) {
        printf("No existing client data file found\n");
        *actual = 0;
        return;
    }
    
    if (fscanf(file, "%d\n", actual) != 1) {
        printf("Error reading actual count\n");
        fclose(file);
        return;
    }
    
    printf("Loading %d clients from file\n", *actual);
    for (int i = 0; i < *actual; i++) {
        clients[i].sock = INVALID_SOCKET;
        clients[i].partie_index = -1;
        clients[i].is_connected = false;
        clients[i].has_pending_request = false;
        
        if (fscanf(file, "%49s\n%d\n%d\n",
            clients[i].name, 
            &clients[i].point,
            &clients[i].has_bio) != 3) {
            printf("Error reading client data for index %d\n", i);
            continue;
        }
        
        printf("Loaded client: %s (points: %d)\n", clients[i].name, clients[i].point);
        
        if (clients[i].has_bio) {
            if (!fgets(clients[i].bio, MAX_BIO_LENGTH, file)) {
                printf("Error reading bio for %s\n", clients[i].name);
                continue;
            }
            clients[i].bio[strcspn(clients[i].bio, "\n")] = 0;
        }
        
        if (fscanf(file, "%d\n", &clients[i].nbFriend) != 1) {
            printf("Error reading friend count for %s\n", clients[i].name);
            continue;
        }
        for (int j = 0; j < clients[i].nbFriend; j++) {
            if (fscanf(file, "%49s\n", clients[i].friend[j]) != 1) {
                printf("Error reading friend %d for %s\n", j, clients[i].name);
                continue;
            }
        }
        
        if (fscanf(file, "%d\n", &clients[i].nbBlock) != 1) {
            printf("Error reading block count for %s\n", clients[i].name);
            continue;
        }
        for (int j = 0; j < clients[i].nbBlock; j++) {
            if (fscanf(file, "%49s\n", clients[i].block[j]) != 1) {
                printf("Error reading blocked user %d for %s\n", j, clients[i].name);
                continue;
            }
        }
    }
    fclose(file);
}