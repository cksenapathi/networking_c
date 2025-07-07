#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 1220
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 50

typedef struct {
    int fd;
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];
    int buffer_pos;
} client_t;

int main(int argc, char* argv[]) {

    int server_fd, max_fd, activity, i, new_socket;
    struct sockaddr_in addr;
    int opt = 1;
    int addrlen = sizeof(addr);
    fd_set set readfds;
    char buffer[BUFFER_SIZE];

    client_t clients[MAX_CLIENTS];


    for(i=0;i<MAX_CLIENTS;i++) {
        clients[i].fd = 0;
        clients[i].buffer_pos = 0;
    }

    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stderr, "socket failed\n");
        return -1;
    }


    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        fprintf(stderr, "setsockopt failed\n");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "bind failed\n");
        return -1;
    }


    if (listen(server_fd, 5)<0) {
       fprintf(stderr, "listen failed");
       return -1;
    }


    fprintf(stdout, "Server listening on port %d (using select())\n", PORT);
    fprintf(stdout, "Waiting for connections...\n");


    while (1) {
        FD_ZERO(&readfds);

        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        for (i=0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd > 0) {
                FD_SET(clients[i].fd, &readfds);
                if (clients[i].fd > max_fd){
                    max_fd = clients[i].fd;
                }
            }
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) {
            fprintf(stderr, "select error");
            break;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen)) < 0) {
                fprintf(stderr, "accept failed\n");
                continue;
            }

            fprintf(stdout, "New connection from %s:%d on socket %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), new_socket);

            for (i=0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == 0) {
                    clients[i].fd = new_socket;
                    clients[i].addr = addr;
                    clients[i].buffer_pos = 0;
                    fprintf(stdout, "Added client to slot %d\n", i);
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Max clients reached, rejecting connection\n.);
                close(new_socket);
            }
        }

        for (i=0;i < MAX_CLIENTS; i++) {
             if (clients[i].fd > 0 && FD_ISSET(clients[i].fd, &readfds)) {
                 memset(buffer, 0, BUFFER_SIZE);
                 ssize_t bytes_read = recv(clients[i].fd, buffer, BUFFER_SIZE - 1, 0);

                 if (bytes_read <= 0) {
                     if (bytes_read == 0) fprintf(stdout, "Client %d disconnected\n", i);
                     else fprintf(stderr, "recv failed\n");
                     close(clients[i].fd);
                     clients[i].fd = 0;
                     clients[i].buffer_pos = 0;
                 } else {
                     fprintf(stdout, "Client %d: %s\n", i, buffer);

                     for (int j = 0; j < MAX_CLIENTS; j++) {
                         if (clients[j].fd > 0 && j != i)
                     }


                 }
             }
        }

    }


    return 0;
}
