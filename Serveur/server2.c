// server.c
#include "server2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>    // For close()
#include <arpa/inet.h> // For inet_ntoa()
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/select.h>
#include "../Awale.h"

void init_server(void)
{
   // No initialization needed for POSIX
}

void close_server(void)
{
   // No cleanup needed for POSIX
}

void run_server(void)
{
   int sock = init_connection();
   int max = sock;
   int actual = 0;
   Client clients[MAX_CLIENTS];
   fd_set rdfs;

   while (1)
   {
      int i;
      FD_ZERO(&rdfs);

      // Add STDIN_FILENO
      FD_SET(STDIN_FILENO, &rdfs);

      // Add the server socket
      FD_SET(sock, &rdfs);

      // Add client sockets
      for (i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
         if (clients[i].sock > max)
            max = clients[i].sock;
      }

      if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      // Something from standard input
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         // Stop the server when input is received
         break;
      }

      // New client connection
      if (FD_ISSET(sock, &rdfs))
      {
         struct sockaddr_in csin;
         socklen_t sinsize = sizeof(csin);
         int csock = accept(sock, (struct sockaddr *)&csin, &sinsize);
         if (csock == -1)
         {
            perror("accept()");
            continue;
         }

         // After connecting, client sends its name
         char buffer[BUF_SIZE];
         int n = read_from_client(csock, buffer);
         if (n <= 0)
         {
            // Client disconnected
            close(csock);
            continue;
         }

         // Add new client to clients array
         Client c = {0};
         c.sock = csock;
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.state = STATE_IDLE;
         c.opponent_sock = -1;
         clients[actual] = c;
         actual++;

         printf("%s connected.\n", c.name);
      }

      // Check clients for incoming messages
      for (i = 0; i < actual; i++)
      {
         if (FD_ISSET(clients[i].sock, &rdfs))
         {
            Client *client = &clients[i];
            char buffer[BUF_SIZE];
            int n = read_from_client(client->sock, buffer);
            if (n == 0)
            {
               // Client disconnected
               printf("%s disconnected.\n", client->name);

               // Notify opponent if in game or waiting
               if (client->state == STATE_BEING_INVITED || client->state == STATE_PLAYING || client->state == STATE_WAITING_FOR_ACCEPTANCE)
               {
                  Client *opponent = find_client_by_sock(clients, actual, client->opponent_sock);
                  if (opponent != NULL)
                  {
                     opponent->state = STATE_IDLE;
                     opponent->opponent_sock = -1;
                     write_to_client(opponent->sock, "Your opponent has disconnected.\n");
                  }
               }

               close(client->sock);
               remove_client(clients, i, &actual);
               i--; // Decrement i since we removed a client
               continue;
            }

            // Handle client message
            handle_client_message(clients, client, &actual, buffer);
         }
      }
   }

   // Cleanup
   for (int i = 0; i < actual; i++)
   {
      close(clients[i].sock);
   }
   close_connection(sock);
}

int init_connection(void)
{
   int sock = socket(AF_INET, SOCK_STREAM, 0);
   struct sockaddr_in sin = {0};

   if (sock == -1)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) == -1)
   {
      perror("bind()");
      close(sock);
      exit(errno);
   }

   if (listen(sock, MAX_CLIENTS) == -1)
   {
      perror("listen()");
      close(sock);
      exit(errno);
   }

   return sock;
}

void close_connection(int sock)
{
   close(sock);
}

int read_from_client(int sock, char *buffer)
{
   int n = recv(sock, buffer, BUF_SIZE - 1, 0);
   if (n <= 0)
   {
      return 0; // Client disconnected
   }
   buffer[n] = '\0';
   return n;
}

void write_to_client(int sock, const char *buffer)
{
   if (send(sock, buffer, strlen(buffer), 0) == -1)
   {
      perror("send()");
      exit(errno);
   }
}

void handle_client_message(Client *clients, Client *sender, int *actual, const char *buffer)
{
   if (strncmp(buffer, "INVITE ", 7) == 0)
   {
      const char *opponent_name = buffer + 7;
      invite_player(clients, sender, *actual, opponent_name);
   }
   else if (strcmp(buffer, "ACCEPT") == 0)
   {
      accept_invitation(clients, sender, *actual);
   }
   else if (strcmp(buffer, "REFUSE") == 0)
   {
      refuse_invitation(clients, sender, *actual);
   }
   else if (strcmp(buffer, "LIST") == 0)
   {
      send_online_players(clients, sender, *actual);
   }
   else if (strncmp(buffer, "MESSAGE", 7) == 0)
   {
      // Move pointer to start after "MESSAGE" prefix
      const char *data = buffer + 7;

      // Read the 2-digit length of the opponent's name
      char length_str[3] = {data[0], data[1], '\0'};
      int opponent_name_len = atoi(length_str);

      // Move pointer forward by 2 to start at the opponent's name
      data += 2;

      // Copy the opponent's name based on the length read
      char opponent_name[opponent_name_len + 1];
      strncpy(opponent_name, data, opponent_name_len);
      opponent_name[opponent_name_len] = '\0'; // Null-terminate the string

      // Move pointer to the actual message content
      data += opponent_name_len;

      // The remaining part of data is the message
      const char *message_content = data;

      // Output for debugging
      printf("Received message from %s: %s\n", opponent_name, message_content);
      send_private_message(clients, sender, *actual, opponent_name, message_content);
   }

   else
   {
      // Broadcast message to all clients except sender
      char message[BUF_SIZE];
      snprintf(message, BUF_SIZE, "%s: %s", sender->name, buffer);
      for (int i = 0; i < *actual; i++)
      {
         if (clients[i].sock != sender->sock)
         {
            write_to_client(clients[i].sock, message);
         }
      }
   }
}
void send_private_message(Client *clients, Client *sender, int actual, const char *opponent_name, const char *mes)
{
   Client *opponent = find_client_by_name(clients, actual, opponent_name);
   if (opponent != NULL && opponent->state == STATE_IDLE)
   {
      // Send message to opponent
      char message[BUF_SIZE];
      snprintf(message, BUF_SIZE, "MESSAGE_FROM %s :%s", sender->name, mes);
      write_to_client(opponent->sock, message);
   }
   else
   {
      write_to_client(sender->sock, "User not available.\n");
   }
}
void invite_player(Client *clients, Client *sender, int actual, const char *opponent_name)
{
   Client *opponent = find_client_by_name(clients, actual, opponent_name);
   if (opponent != NULL && opponent->state == STATE_IDLE)
   {
      // Send invitation to opponent
      char message[BUF_SIZE];
      snprintf(message, BUF_SIZE, "INVITE_FROM %s", sender->name);
      write_to_client(opponent->sock, message);

      sender->state = STATE_WAITING_FOR_ACCEPTANCE;
      sender->opponent_sock = opponent->sock;

      opponent->state = STATE_BEING_INVITED;
      opponent->opponent_sock = sender->sock;
   }
   else
   {
      write_to_client(sender->sock, "User not available.\n");
   }
}

void accept_invitation(Client *clients, Client *sender, int actual)
{
   if (sender->state != STATE_BEING_INVITED)
   {
      write_to_client(sender->sock, "No invitation to accept.\n");
      return;
   }

   Client *inviter = find_client_by_sock(clients, actual, sender->opponent_sock);
   if (inviter != NULL && inviter->state == STATE_WAITING_FOR_ACCEPTANCE)
   {
      // Notify inviter
      write_to_client(inviter->sock, "ACCEPTED");
      sender->state = STATE_PLAYING;
      inviter->state = STATE_PLAYING;

      // Start game between inviter and sender
      int random = random_choice();
      if (random == 1)
      {
         start_game(inviter, sender);
      }
      else
      {
         start_game(sender, inviter);
      }
   }
   else
   {
      write_to_client(sender->sock, "Inviter not found.\n");
      sender->state = STATE_IDLE;
      sender->opponent_sock = -1;
   }
}

int random_choice()
{
   return (rand() % 2) + 1;
}

void refuse_invitation(Client *clients, Client *sender, int actual)
{
   if (sender->state != STATE_BEING_INVITED)
   {
      write_to_client(sender->sock, "No invitation to refuse.\n");
      return;
   }

   Client *inviter = find_client_by_sock(clients, actual, sender->opponent_sock);
   if (inviter != NULL && inviter->state == STATE_WAITING_FOR_ACCEPTANCE)
   {
      // Notify inviter
      write_to_client(inviter->sock, "REFUSED");

      // Reset states
      inviter->state = STATE_IDLE;
      inviter->opponent_sock = -1;

      sender->state = STATE_IDLE;
      sender->opponent_sock = -1;
   }
   else
   {
      write_to_client(sender->sock, "Inviter not found.\n");
      sender->state = STATE_IDLE;
      sender->opponent_sock = -1;
   }
}

char *getBoardString(Game *game)
{
   static char boardStr[BUF_SIZE];
   char temp[64];
   boardStr[0] = '\0';

   strcat(boardStr, "\n+-------------------------------+\n");
   strcat(boardStr, "|        Player 2's Side        |\n");
   strcat(boardStr, "+-------------------------------+\n");

   // Top row (Player 2's pits)
   strcat(boardStr, "| ");
   for (int i = 11; i >= 6; i--)
   {
      snprintf(temp, sizeof(temp), " %2d ", game->board[i]);
      strcat(boardStr, temp);
   }
   strcat(boardStr, " |\n");

   strcat(boardStr, "+-------------------------------+\n");

   // Bottom row (Player 1's pits)
   strcat(boardStr, "| ");
   for (int i = 0; i <= 5; i++)
   {
      snprintf(temp, sizeof(temp), " %2d ", game->board[i]);
      strcat(boardStr, temp);
   }
   strcat(boardStr, " |\n");

   strcat(boardStr, "+-------------------------------+\n");
   strcat(boardStr, "|        Player 1's Side        |\n");
   strcat(boardStr, "+-------------------------------+\n");

   return boardStr;
}

void start_game(Client *player1, Client *player2)
{
   Game game;
   initBoard(&game);

   int currentPlayer = 1;
   int game_over = 0;
   char buffer[BUF_SIZE];

   // Notify both players that the game has started
   write_to_client(player1->sock, "Game started! You are Player 1.\n");
   write_to_client(player2->sock, "Game started! You are Player 2.\n");

   while (!gameFinished(&game))
   {
      // Reinitialize buffer at the start of each loop iteration
      buffer[0] = '\0';

      // Send the current game board to both players
      snprintf(buffer, BUF_SIZE, "Current Board:\n%s\n", getBoardString(&game));

      write_to_client(player1->sock, buffer);
      write_to_client(player2->sock, buffer);

      Client *currentPlayerClient = (currentPlayer == 1) ? player1 : player2;
      Client *waitingPlayerClient = (currentPlayer == 1) ? player2 : player1;
      buffer[0] = '\0';

      // Prompt the current player to choose a pit
      snprintf(buffer, BUF_SIZE, "Your turn, Player %d. Choose a pit (%d-%d):\n", currentPlayer, (currentPlayer - 1) * 6, currentPlayer * 6 - 1);
      write_to_client(currentPlayerClient->sock, buffer);

      // Inform the waiting player to wait
      snprintf(buffer, BUF_SIZE, "Waiting for Player %d to make a move...\n", currentPlayer);
      write_to_client(waitingPlayerClient->sock, buffer);

      // Read the pit choice from the current player
      int pit = -1;
      int valid_input = 0;
      while (!valid_input)
      {
         // Reinitialize buffer for each new input read
         buffer[0] = '\0';

         int n = read_from_client(currentPlayerClient->sock, buffer);
         if (n <= 0)
         {
            // Player disconnected
            write_to_client(waitingPlayerClient->sock, "Your opponent has disconnected. Game over.\n");
            currentPlayerClient->state = STATE_IDLE;
            waitingPlayerClient->state = STATE_IDLE;
            return;
         }

         // Try to parse the input as an integer
         if (sscanf(buffer, "%d", &pit) != 1)
         {
            write_to_client(currentPlayerClient->sock, "Invalid input. Please enter a number.\n");
            continue;
         }

         // Check if the pit number is valid
         int pit_start = (currentPlayer - 1) * 6;
         int pit_end = currentPlayer * 6 - 1;
         if (pit >= pit_start && pit <= pit_end)
         {
            if (play(&game, currentPlayer, pit))
            {
               valid_input = 1;
            }
            else
            {
               write_to_client(currentPlayerClient->sock, "Invalid move. The pit is empty. Choose another pit.\n");
            }
         }
         else
         {
            write_to_client(currentPlayerClient->sock, "Invalid pit number. Try again.\n");
         }
      }

      // Move to the next player
      currentPlayer = (currentPlayer == 1) ? 2 : 1;
   }

   // Game is over, send final scores
   buffer[0] = '\0'; // Clear buffer
   const char *blue = "\033[1;34m";
   const char *green = "\033[1;32m";
   const char *reset = "\033[0m";

   snprintf(buffer, BUF_SIZE, "Game over!\nFinal scores:\n%sPlayer 1: %d%s\n%sPlayer 2: %d%s\n",
            blue, game.scoreFirstPlayer, reset, green, game.scoreSecondPlayer, reset);
   write_to_client(player1->sock, buffer);
   write_to_client(player2->sock, buffer);

   if (game.scoreFirstPlayer > game.scoreSecondPlayer)
   {
      snprintf(buffer, BUF_SIZE, "%sYou win! Congratulations!%s\n", (game.scoreFirstPlayer > game.scoreSecondPlayer) ? blue : green, reset);
      write_to_client(player1->sock, buffer);
      snprintf(buffer, BUF_SIZE, "%sYou lose.%s\n", (game.scoreSecondPlayer < game.scoreFirstPlayer) ? green : blue, reset);
      write_to_client(player2->sock, buffer);
   }
   else if (game.scoreSecondPlayer > game.scoreFirstPlayer)
   {
      snprintf(buffer, BUF_SIZE, "%sYou win! Congratulations!%s\n", (game.scoreSecondPlayer > game.scoreFirstPlayer) ? green : blue, reset);
      write_to_client(player2->sock, buffer);
      snprintf(buffer, BUF_SIZE, "%sYou lose.%s\n", (game.scoreFirstPlayer < game.scoreSecondPlayer) ? blue : green, reset);
      write_to_client(player1->sock, buffer);
   }
   else
   {
      write_to_client(player1->sock, "It's a tie!\n");
      write_to_client(player2->sock, "It's a tie!\n");
   }

   // Reset player states
   player1->state = STATE_IDLE;
   player1->opponent_sock = -1;

   player2->state = STATE_IDLE;
   player2->opponent_sock = -1;
}

Client *find_client_by_name(Client *clients, int actual, const char *name)
{
   for (int i = 0; i < actual; i++)
   {
      if (strcmp(clients[i].name, name) == 0)
      {
         return &clients[i];
      }
   }
   return NULL;
}

Client *find_client_by_sock(Client *clients, int actual, int sock)
{
   for (int i = 0; i < actual; i++)
   {
      if (clients[i].sock == sock)
      {
         return &clients[i];
      }
   }
   return NULL;
}

void remove_client(Client *clients, int to_remove, int *actual)
{
   // Remove client from array
   memmove(&clients[to_remove], &clients[to_remove + 1], (*actual - to_remove - 1) * sizeof(Client));
   (*actual)--;
}

void send_online_players(Client *clients, Client *sender, int actual)
{
   char message[BUF_SIZE] = "Online players:\n";
   for (int i = 0; i < actual; i++)
   {
      if (clients[i].sock != sender->sock)
      {
         strcat(message, clients[i].name);
         strcat(message, "\n");
      }
   }
   write_to_client(sender->sock, message);
}

int main()
{
   init_server();
   run_server();
   close_server();
   return 0;
}
