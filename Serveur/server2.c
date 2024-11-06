#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server2.h"

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

static void init(void) {
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if(err < 0) {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

static void end(void) {
    pthread_mutex_destroy(&clients_mutex);
    pthread_mutex_destroy(&socket_mutex);
#ifdef WIN32
    WSACleanup();
#endif
}

// Modifications dans la fonction check_pseudo dans server2.c
static int check_pseudo(Client *clients, int actual, const char *pseudo) {
    // Vérification de la longueur
    size_t len = strlen(pseudo);
    if(len < PSEUDO_MIN_LENGTH || len >= PSEUDO_MAX_LENGTH) {
        return 0;
    }

    // Vérification des caractères valides (lettres, chiffres, underscore)
    for(size_t i = 0; i < len; i++) {
        if(!isalnum(pseudo[i]) && pseudo[i] != '_') {
            return 0;
        }
    }

    // Vérification de l'unicité
    for(int i = 0; i < actual; i++) {
        if(strcasecmp(clients[i].name, pseudo) == 0) {  // Case-insensitive comparison
            return 0;
        }
    }
    
    return 1;
}

static void handle_bio_command(Client *clients, int actual, int client_index, const char *buffer) {
    char response[BUF_SIZE];
    
    if(strncmp(buffer, "setbio:", 7) == 0) {
        strncpy(clients[client_index].bio, buffer + 7, MAX_BIO_LENGTH - 1);
        clients[client_index].has_bio = 1;
        strcpy(response, "Bio updated successfully!\n");
        write_client(clients[client_index].sock, response);
    }
    else if(strncmp(buffer, "getbio:", 7) == 0) {
        char target_pseudo[BUF_SIZE];
        strncpy(target_pseudo, buffer + 7, BUF_SIZE - 1);
        
        int found = 0;
        for(int i = 0; i < actual; i++) {
            if(strcmp(clients[i].name, target_pseudo) == 0) {
                if(clients[i].has_bio) {
                    snprintf(response, BUF_SIZE, "Bio of %s:\n%s\n", target_pseudo, clients[i].bio);
                } else {
                    snprintf(response, BUF_SIZE, "%s hasn't set their bio yet.\n", target_pseudo);
                }
                write_client(clients[client_index].sock, response);
                found = 1;
                break;
            }
        }
        
        if(!found) {
            snprintf(response, BUF_SIZE, "User %s not found.\n", target_pseudo);
            write_client(clients[client_index].sock, response);
        }
    }
}

static void handle_awale_command(Client *clients, int actual, int client_index, const char *buffer) {
    char target_pseudo[BUF_SIZE];
    strncpy(target_pseudo, buffer + 6, BUF_SIZE - 1);
    
    int found = 0;
    for(int i = 0; i < actual; i++) {
        if(strcmp(clients[i].name, target_pseudo) == 0) {
            char response[BUF_SIZE];
            snprintf(response, BUF_SIZE, "Awale fight request from %s\n", clients[client_index].name);
            write_client(clients[i].sock, response);
            found = 1;
            break;
        }
    }
    
    if(!found) {
        char response[BUF_SIZE];
        snprintf(response, BUF_SIZE, "User %s not found.\n", target_pseudo);
        write_client(clients[client_index].sock, response);
    }
}

static void handle_private_message(Client *clients, int actual, int sender_index, const char *buffer) {
    char target_pseudo[BUF_SIZE];
    char message[BUF_SIZE];
    const char *msg_start = strchr(buffer + 3, ':');
    
    if(msg_start == NULL) return;
    
    int pseudo_len = msg_start - (buffer + 3);
    strncpy(target_pseudo, buffer + 3, pseudo_len);
    target_pseudo[pseudo_len] = '\0';
    
    strncpy(message, msg_start + 1, BUF_SIZE - 1);
    
    for(int i = 0; i < actual; i++) {
        if(strcmp(clients[i].name, target_pseudo) == 0) {
            char formatted_msg[BUF_SIZE];
            snprintf(formatted_msg, BUF_SIZE, "[Private from %s]: %s\n", 
                    clients[sender_index].name, message);
            write_client(clients[i].sock, formatted_msg);
            
            snprintf(formatted_msg, BUF_SIZE, "[Private to %s]: %s\n", 
                    target_pseudo, message);
            write_client(clients[sender_index].sock, formatted_msg);
            return;
        }
    }
    
    char error_msg[BUF_SIZE];
    snprintf(error_msg, BUF_SIZE, "User %s not found.\n", target_pseudo);
    write_client(clients[sender_index].sock, error_msg);
}

static void list_connected_clients(Client *clients, int actual, int requester_index) {
    char list[BUF_SIZE * MAX_CLIENTS] = "Connected users:\n";
    
    for(int i = 0; i < actual; i++) {
        if(clients[i].sock != INVALID_SOCKET) {
            char user_entry[BUF_SIZE];
            snprintf(user_entry, BUF_SIZE, "- %s\n", clients[i].name);
            strcat(list, user_entry);
        }
    }
    
    write_client(clients[requester_index].sock, list);
}

static void app(void) {
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    int actual = 0;
    int max = sock;
    Client clients[MAX_CLIENTS];
    fd_set rdfs;
    
    while(1) {
        int i = 0;
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);
        
        for(i = 0; i < actual; i++) {
            FD_SET(clients[i].sock, &rdfs);
        }
        
        if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            exit(errno);
        }
        
        if(FD_ISSET(STDIN_FILENO, &rdfs)) {
            break;
        }
        else if(FD_ISSET(sock, &rdfs)) {
            SOCKADDR_IN csin = { 0 };
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            
            if(csock == SOCKET_ERROR) {
               perror("accept()");
               continue;
            }

            if(actual >= MAX_CLIENTS) {
               const char *msg = "Server is full, please try again later.\n";
               write_client(csock, msg);
               closesocket(csock);
               continue;
            }
            
            int read_size = read_client(csock, buffer);
            if(read_size <= 0) {
               closesocket(csock);
               continue;
            }
            
            buffer[read_size] = 0; // Ensure null termination
            
            // Vérification du pseudo
            if(!check_pseudo(clients, actual, buffer)) {
               const char *error_msg;
               if(strlen(buffer) < PSEUDO_MIN_LENGTH) {
                     error_msg = "pseudo_too_short";
               } else if(strlen(buffer) >= PSEUDO_MAX_LENGTH) {
                     error_msg = "pseudo_too_long";
               } else {
                     error_msg = "pseudo_exists";
               }
               write_client(csock, error_msg);
               closesocket(csock);
               continue;
            }
            
            // Si on arrive ici, le pseudo est valide
            max = csock > max ? csock : max;
            
            Client c = { csock };
            strncpy(c.name, buffer, PSEUDO_MAX_LENGTH - 1);
            c.name[PSEUDO_MAX_LENGTH - 1] = 0;  // Ensure null termination
            c.has_bio = 0;
            clients[actual] = c;
            actual++;
            
            write_client(csock, "connected");
            
            // Inform other clients
            char connect_msg[BUF_SIZE];
            snprintf(connect_msg, BUF_SIZE, "%s has joined the chat!\n", buffer);
            send_message_to_all_clients(clients, c, actual, connect_msg, 1);
         }
        else {
            int i = 0;
            for(i = 0; i < actual; i++) {
                if(FD_ISSET(clients[i].sock, &rdfs)) {
                    Client client = clients[i];
                    int c = read_client(clients[i].sock, buffer);
                    
                    if(c == 0) {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual);
                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " disconnected!\n", BUF_SIZE - strlen(buffer) - 1);
                        send_message_to_all_clients(clients, client, actual, buffer, 1);
                    }
                    else {
                        if(strncmp(buffer, "list:", 5) == 0) {
                            list_connected_clients(clients, actual, i);
                        }
                        else if(strncmp(buffer, "mp:", 3) == 0) {
                            handle_private_message(clients, actual, i, buffer);
                        }
                        else if(strncmp(buffer, "setbio:", 7) == 0 || 
                                strncmp(buffer, "getbio:", 7) == 0) {
                            handle_bio_command(clients, actual, i, buffer);
                        }
                        else if (strncmp(buffer, "awale:", 6) == 0) {
                            handle_awale_command(clients, actual, i, buffer);
                        }
                        else {
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

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

int main(int argc, char **argv)
{
   
   init();
   
   printf("Server started on port %d\n", PORT);
   app();

   end();

   return EXIT_SUCCESS;
}
