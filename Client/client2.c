
// client2.c
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "client2.h"

static void clear_screen(void) {
#ifdef WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void print_menu(void) {
    printf("\n\033[1;36m=== Chat Menu ===\033[0m\n");
    printf("1. Send message to all\n");
    printf("2. Send private message\n");
    printf("3. List connected users\n");
    printf("4. Bio options\n");
    printf("5. Play awale vs someone\n");
    printf("6. Clear screen\n");
    printf("7. Quit\n");
    printf("\nChoice: ");
    fflush(stdout);
}

static void get_multiline_input(char *buffer, int max_size) {
    printf("Enter your text (end with an empty line):\n");
    memset(buffer, 0, max_size);
    char line[256];
    int total_len = 0;
    
    while (1) {
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        if (line[0] == '\n') break;
        
        int line_len = strlen(line);
        if (total_len + line_len >= max_size - 1) break;
        
        strcat(buffer, line);
        total_len += line_len;
    }
}

static int get_valid_pseudo(SOCKET sock) {
    char buffer[BUF_SIZE];
    char response[BUF_SIZE];
    
    while (1) {
        printf("\033[1;33mEnter your nickname (2-%d characters, letters, numbers, underscore only): \033[0m", 
               PSEUDO_MAX_LENGTH - 1);
        fflush(stdout);
        
        if (fgets(buffer, BUF_SIZE - 1, stdin) == NULL) {
            return 0;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;
        
        write_server(sock, buffer);
        
        if (read_server(sock, response) == -1) {
            return 0;
        }
        
        if (strcmp(response, "connected") == 0) {
            printf("\033[1;32mConnected successfully!\033[0m\n");
            return 1;
        } else if (strcmp(response, "pseudo_exists") == 0) {
            printf("\033[1;31mThis nickname is already taken. Please choose another one.\033[0m\n");
            close(sock);
            exit(EXIT_FAILURE);
        } else if (strcmp(response, "pseudo_too_short") == 0) {
            printf("\033[1;31mNickname too short (minimum 2 characters).\033[0m\n");
            close(sock);
            exit(EXIT_FAILURE);
        } else if (strcmp(response, "pseudo_too_long") == 0) {
            printf("\033[1;31mNickname too long (maximum %d characters).\033[0m\n", PSEUDO_MAX_LENGTH - 1);
            close(sock);
            exit(EXIT_FAILURE);
        } else {
            printf("\033[1;31mInvalid nickname format. Use letters, numbers, and underscore only.\033[0m\n");
            close(sock);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}

static void handle_user_input(SOCKET sock) {
    char buffer[BUF_SIZE];
    char input[BUF_SIZE];
    int choice;
    
    if(fgets(input, BUF_SIZE, stdin) == NULL) return;
    choice = atoi(input);
    
    switch(choice) {
        case 1: // Send public message
            printf("\033[1;34mEnter your message: \033[0m");
            if(fgets(buffer, BUF_SIZE - 1, stdin) != NULL) {
                buffer[strcspn(buffer, "\n")] = 0;
                write_server(sock, buffer);
            }
            break;
            
        case 2: // Send private message
            {
                char dest[BUF_SIZE];
                char msg[BUF_SIZE];
                printf("\033[1;34mEnter recipient's nickname: \033[0m");
                if(fgets(dest, BUF_SIZE - 1, stdin) == NULL) break;
                dest[strcspn(dest, "\n")] = 0;
                
                printf("\033[1;34mEnter your message: \033[0m");
                if(fgets(msg, BUF_SIZE - 1, stdin) == NULL) break;
                msg[strcspn(msg, "\n")] = 0;
                
                snprintf(buffer, BUF_SIZE, "mp:%s:%s", dest, msg);
                write_server(sock, buffer);
            }
            break;
            
        case 3: // List users
            strcpy(buffer, "list:");
            write_server(sock, buffer);
            break;
            
        case 4: // Bio options
            printf("\n\033[1;36m=== Bio Options ===\033[0m\n");
            printf("1. Set your bio\n");
            printf("2. View someone's bio\n");
            printf("Choice: ");
            
            if(fgets(input, BUF_SIZE, stdin) != NULL) {
                int bio_choice = atoi(input);
                if(bio_choice == 1) {
                    char bio[MAX_BIO_LENGTH] = "setbio:";
                    printf("\033[1;34mEnter your bio:\033[0m\n");
                    get_multiline_input(bio + 7, MAX_BIO_LENGTH - 7);
                    write_server(sock, bio);
                }
                else if(bio_choice == 2) {
                    printf("\033[1;34mEnter nickname to view bio: \033[0m");
                    if(fgets(input, BUF_SIZE - 1, stdin) != NULL) {
                        input[strcspn(input, "\n")] = 0;
                        snprintf(buffer, BUF_SIZE, "getbio:%s", input);
                        write_server(sock, buffer);
                    }
                }
            }
            break;
        case 5: // Play awale
            printf("\033[1;34mEnter the nickname of the player you want to play against: \033[0m");
            if(fgets(buffer, BUF_SIZE - 1, stdin) != NULL) {
                buffer[strcspn(buffer, "\n")] = 0;
                snprintf(input, BUF_SIZE, "awale:%s", buffer);
                write_server(sock, input);
                printf("\033[1;33mWaiting for response...\033[0m\n");
            }
            break;
            
        case 6: // Clear screen
            clear_screen();
            break;
            
        case 7: // Quit
            printf("\033[1;33mGoodbye!\033[0m\n");
            exit(EXIT_SUCCESS);
            break;
            
        default:
            printf("\033[1;31mInvalid choice.\033[0m\n");
            break;
    }
}

static void app(const char *address, const char *name) {
    SOCKET sock = init_connection(address);
    char buffer[BUF_SIZE];
    fd_set rdfs;
    
    /* get valid pseudo */
    if (!get_valid_pseudo(sock)) {
        printf("\033[1;31mError getting valid nickname\033[0m\n");
        exit(EXIT_FAILURE);
    }
    
    clear_screen();
    print_menu();
    
    while(1) {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);
        
        if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1) {
            perror("select()");
            exit(errno);
        }
        
        if(FD_ISSET(STDIN_FILENO, &rdfs)) {
            handle_user_input(sock);
            print_menu();
        }
        else if(FD_ISSET(sock, &rdfs)) {
            int n = read_server(sock, buffer);
            if(n == 0) {
                printf("\033[1;31mServer disconnected!\033[0m\n");
                break;
            }
            
            // Clear the current line and move cursor up
            printf("\r\033[K\033[A\033[K");
            
            // Print the message with appropriate color based on type
            if(strstr(buffer, "[Private") != NULL) {
                printf("\033[1;35m%s\033[0m\n", buffer); // Magenta for private messages
            }
            else if(strstr(buffer, "joined") != NULL || strstr(buffer, "disconnected") != NULL) {
                printf("\033[1;33m%s\033[0m\n", buffer); // Yellow for system messages
            }
            else if(strstr(buffer, "fight") != NULL) {
                printf("\033[1;31m%s\033[0m\n", buffer); // Red for fight messages
                printf("yes or no ?\n");
            }
            else {
                printf("\033[1;32m%s\033[0m\n", buffer); // Green for normal messages
            }
            
            print_menu();
        }
    }
    
    end_connection(sock);
}

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
#ifdef WIN32
    WSACleanup();
#endif
}

static int init_connection(const char *address) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    struct hostent *hostinfo;
    
    if(sock == INVALID_SOCKET) {
        perror("socket()");
        exit(errno);
    }
    
    hostinfo = gethostbyname(address);
    if(hostinfo == NULL) {
        fprintf(stderr, "Unknown host %s.\n", address);
        exit(EXIT_FAILURE);
    }
    
    sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;
    
    if(connect(sock, (SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        perror("connect()");
        exit(errno);
    }
    
    return sock;
}

static void end_connection(int sock) {
    closesocket(sock);
}

static int read_server(SOCKET sock, char *buffer) {
    int n = 0;
    
    if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
        perror("recv()");
        exit(errno);
    }
    
    buffer[n] = 0;
    return n;
}

static void write_server(SOCKET sock, const char *buffer) {
    if(send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("send()");
        exit(errno);
    }
}

int main(int argc, char **argv) {
    if(argc < 2) {
        printf("Usage: %s [address]\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    init();
    app(argv[1], argv[2]);
    end();
    
    return EXIT_SUCCESS;
}