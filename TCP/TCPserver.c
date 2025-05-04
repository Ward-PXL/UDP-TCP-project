#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 8888
#define MAX_CLIENTS  FD_SETSIZE
#define BUFFER_SIZE  32

typedef struct {
    int socket;
    int target;
} ClientData;

void handle_guess(ClientData *client, int guess) {
    if (guess < client->target) {
        send(client->socket, "Hoger", 5, 0);
    } else if (guess > client->target) {
        send(client->socket, "Lager", 5, 0);
    } else {
        send(client->socket, "Correct", 7, 0);
        client->target = rand() % 1000000 + 1; // nieuwe ronde
    }
}

int main() {
    int server_fd, new_socket, max_sd, activity, sd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    ClientData clients[MAX_CLIENTS] = {0};
    fd_set readfds;

    srand(time(NULL));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 10);
    printf("TCP-server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        // Nieuwe connectie
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                perror("accept");
                continue;
            }

            printf("New connection: socket %d, IP %s\n", new_socket, inet_ntoa(address.sin_addr));

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket == 0) {
                    clients[i].socket = new_socket;
                    clients[i].target = rand() % 1000000 + 1;
                    break;
                }
            }
        }

        // Clientgegevens verwerken
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].socket;

            if (FD_ISSET(sd, &readfds)) {
                int guess_net;
                int valread = recv(sd, &guess_net, sizeof(guess_net), 0);

                if (valread <= 0) {
                    printf("Client disconnected: socket %d\n", sd);
                    close(sd);
                    clients[i].socket = 0;
                } else {
                    int guess = ntohl(guess_net);
                    if (guess == -1) {
                        printf("Client exited game: socket %d\n", sd);
                        close(sd);
                        clients[i].socket = 0;
                    } else {
                        handle_guess(&clients[i], guess);
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
