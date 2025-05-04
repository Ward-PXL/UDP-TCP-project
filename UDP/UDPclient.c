#ifdef _WIN32
    #define _WIN32_WINNT _WIN32_WINNT_WIN7
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
#endif

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,0), &wsaData);
#endif

    char ip[100];
    printf("Server IP: ");
    scanf("%s", ip);

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(ip, "8888", &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

#ifdef _WIN32
    DWORD timeout = 16000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 16;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#endif

    char msg[1000], buf[1000];
    socklen_t addrlen = (socklen_t)res->ai_addrlen;

    while (1) {
        printf("Enter guess: ");
        scanf("%s", msg);

        sendto(sock, msg, strlen(msg), 0, res->ai_addr, res->ai_addrlen);

        int recv_len = recvfrom(sock, buf, sizeof(buf) - 1, 0, res->ai_addr, &addrlen);
#ifdef _WIN32
        if (recv_len == SOCKET_ERROR) {
#else
        if (recv_len < 0) {
#endif
            printf("You lost ?\n");
        } else {
            buf[recv_len] = '\0';
            printf("Server: %s\n", buf);
        }
    }

    close(sock);
    freeaddrinfo(res);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;

    }
}
