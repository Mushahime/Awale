#ifndef PERSISTANCE_H
#define PERSISTANCE_H

#include "utilsServer.h"
#define DATA_FILE "client_data.txt"

void save_client_data(Client *clients, int actual);
void load_client_data(Client *clients, int *actual);

#endif