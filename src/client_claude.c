#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {

    if (argc != 2) {
        fprintf(stderr, "usage: server_claude port\n");
        return -1;
    }

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE]={0};
    char message[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));

    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address / Address not supported\n");
        close(sock);
        return -1;
    }

    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Connection failed\n");
        close(sock);
        return -1;
    }

    fprintf(stdout, "connected to server. type message (or 'quit' to exit):\n");

    while (1) {
        fprintf(stdout, "> ");
        fflush(stdout);

        if (!fgets(message, sizeof(message), stdin)) {
            break;
        }

        if (strncmp(message, "quit", 4) == 0) {
            break;
        }

        if (send(sock, message, strlen(message), 0) == -1) {
            fprintf(stderr, "Message send failed\n");
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) fprintf(stdout, "Server disconnected\n");
            else fprintf(stderr, "recv failed");
            break;
        }

        fprintf(stdout, "Echo: %s\n", buffer);
    }

    close(sock);
    fprintf(stdout, "Disconnected from server\n");
    return 0;
}
