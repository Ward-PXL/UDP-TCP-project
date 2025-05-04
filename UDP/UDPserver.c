#ifdef _WIN32
    #define _WIN32_WINNT _WIN32_WINNT_WIN7
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>
    #define close closesocket
    void OSInit() {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2,0), &wsaData);
        if (result != 0) {
            fprintf(stderr, "WSAStartup failed\n");
            exit(1);
        }
    }
    void OSCleanup() { WSACleanup(); }
#else
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <time.h>
    void OSInit() {}
    void OSCleanup() {}
#endif

int initialization();
void execution(int socket);
void cleanup(int socket);

int main() {
    OSInit();
    int sock = initialization();
    execution(sock);
    cleanup(sock);
    OSCleanup();
    return 0;
}

int initialization() {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "8888", &hints, &res) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        close(sock);
        exit(1);
    }

    freeaddrinfo(res);
    return sock;
}

void execution(int sock) {
    srand(time(NULL));
    int number = rand() % 100 + 1;
    printf("New game started. Number: %d\n", number);

    while (1) {
        int best_diff = 9999, best_guess = -1;
        char buffer[2000];
        struct sockaddr_storage client_addr;
        socklen_t client_len = sizeof(client_addr);

        int timeout = 8000;

        while (1) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(sock, &fds);

            struct timeval tv;
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;

            printf("Waiting for guesses (%dms)...\n", timeout);

            int activity = select(sock + 1, &fds, NULL, NULL, &tv);
            if (activity <= 0) break;

            int recv_len = recvfrom(sock, buffer, sizeof(buffer)-1, 0,
                                    (struct sockaddr *)&client_addr, &client_len);
            if (recv_len < 0) continue;

            buffer[recv_len] = '\0';
            int guess = atoi(buffer);
            int diff = abs(guess - number);

            printf("Received %d\n", guess);
            if (diff < best_diff) {
                best_diff = diff;
                best_guess = guess;
            }

            timeout = timeout / 2 > 500 ? timeout / 2 : 500; // halve timeout
        }

        char msg[50];
        if (best_diff == 0) {
            sprintf(msg, "You won !");
        } else {
            sprintf(msg, "You won ?");
        }

        sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&client_addr, client_len);

        // Late guesses
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        tv.tv_sec = 16;
        tv.tv_usec = 0;

        if (select(sock + 1, &fds, NULL, NULL, &tv) > 0) {
            recvfrom(sock, buffer, sizeof(buffer)-1, 0,
                     (struct sockaddr *)&client_addr, &client_len);
            sendto(sock, "You lost !", 10, 0, (struct sockaddr *)&client_addr, client_len);
        }

        number = rand() % 100 + 1;
        printf("New game started. Number: %d\n", number);
    }
}

void cleanup(int sock) {
    close(sock);
}
