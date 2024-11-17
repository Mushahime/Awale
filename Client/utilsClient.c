#include "utilsClient.h"
 void init(void) {
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if(err < 0) {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

 void end(void) {
#ifdef WIN32
    WSACleanup();
#endif
}

 int init_connection(const char *address) {
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

 void end_connection(int sock) {
    closesocket(sock);
}

 int read_server(SOCKET sock, char *buffer) {
    int n = 0;
    
    if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
        perror("recv()");
        exit(errno);
    }
    
    buffer[n] = 0;
    return n;
}

 void write_server(SOCKET sock, const char *buffer) {
    if(send(sock, buffer, strlen(buffer), 0) < 0) {
        perror("send()");
        exit(errno);
    }
}
 void clear_screen(void) {
#ifdef WIN32
    system("cls");
#else
    system("clear");
#endif
}

 void print_menu(void) {
    printf("\n\033[1;36m=== Chat Menu ===\033[0m\n");
    printf("1. Send message to all\n");
    printf("2. Send private message\n");
    printf("3. List connected users\n");
    printf("4. Bio options\n");
    printf("5. Play awale vs someone\n");
    printf("6. ALl games in progression\n");
    printf("7. Clear screen\n");
    printf("8. Quit\n");
    printf("\n");
    printf("\nChoice: ");
    fflush(stdout);
}
