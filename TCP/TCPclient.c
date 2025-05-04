#ifdef _WIN32
#define _WIN32_WINNT 0x0601
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    char server_ip[100];
    printf("Enter server IP: ");
    scanf("%99s", server_ip);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_ip, "8888", &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Guess numbers between 1 and 1000000.\n");

    while (1) {
        int guess;
        printf("Enter your guess (-1 to quit): ");
        scanf("%d", &guess);

        int net_guess = htonl(guess);
        if (send(sock, &net_guess, sizeof(net_guess), 0) <= 0) {
            perror("send");
            break;
        }

        if (guess == -1) break;

        char response[32];
        int bytes = recv(sock, response, sizeof(response) - 1, 0);
        if (bytes <= 0) {
            printf("Disconnected from server.\n");
            break;
        }
        response[bytes] = '\0';
        printf("Server says: %s\n", response);

        if (strcmp(response, "Correct") == 0) {
            printf("You won! Starting new round...\n");
        }
    }

    close(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
} 
