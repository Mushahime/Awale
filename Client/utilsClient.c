#include "utilsClient.h"
#include <string.h>
#include <string.h>
#include <fcntl.h>

// Function to initialize the program
void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

// Function to end the program
void end(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}

// Function to initialize the connection
int init_connection(const char *address, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("socket()");
        exit(errno);
    }

    SOCKADDR_IN sin = {0};
    struct hostent *hostinfo = gethostbyname(address);
    if (hostinfo == NULL) {
        fprintf(stderr, "Unknown host %s.\n", address);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;

    if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        perror("connect()");
        closesocket(sock);
        exit(errno);
    }

    // Set socket as non-blocking
    #ifdef WIN32
        unsigned long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
    #else
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    #endif

    return sock;
}

// Function to end a connection
void end_connection(int sock)
{
    closesocket(sock);
}

// Function to read from a server
int read_server(SOCKET sock, char *buffer)
{
    int n;
    while ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            usleep(100000);  // 100ms
            continue;
        }
        perror("recv()");
        return -1;
    }

    buffer[n] = 0;
    return n;
}

// Function to write to a server
void write_server(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}
void clear_screen(void)
{
#ifdef WIN32
    system("cls");
#else
    system("clear");
#endif
}