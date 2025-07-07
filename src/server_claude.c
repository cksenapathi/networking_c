#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "usage: server port\n");
        return -1;
    }


    int server_fd, client_fd;
    struct sockaddr_in addr;
    int opt = 1;
    size_t addrlen = sizeof(addr);
    char buffer[BUFFER_SIZE] = {0};

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        fprintf(stdout, "socket failed\n");
        return -1;
    }


    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        fprintf(stderr, "setsockopt failed");
        close(server_fd);
        return -1;
    }


    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));


    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "bind_failed\n");
        close(server_fd);
        return -1;
    }

    
    if(listen(server_fd, 5) < 0) {
        fprintf(stderr, "listen failed\n");
        close(server_fd);
        return -1;
    }


    fprintf(stdout, "Server listening on port %d...\n", atoi(argv[1]));


    while (1) {
        fprintf(stdout, "Waiting for connection...\n");
        
        if((client_fd = accept(server_fd, (struct sockaddr*)&addr, (socklen_t*)&addrlen)) < 0) {
            fprintf(stderr, "Accept failed\n");
            continue;
        }

        fprintf(stdout, "Client connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        while(1) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t bytes_read = recv(client_fd, buffer, BUFFER_SIZE-1, 0);


            if (bytes_read <= 0) {
                if (bytes_read == 0) fprintf(stdout, "Client disconnected.\n");
                else fprintf(stderr, "recv failed");
                break;
            }

            fprintf(stdout, "Received: %s\n", buffer);


            if (send(client_fd, buffer, bytes_read, 0) == -1) {
                fprintf(stderr, "send failed\n");
                break;
            }

        }

        close(client_fd);
        fprintf(stdout, "Connection closed");

    }

    close(server_fd);
    return 0;
}
