#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024


void* receive_messages(void* arg) {
    int sock = *(int*)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;


    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            if (bytes_received == 0) fprintf(stdout, "\nServer disconnected\n");
            else fprintf(stderr, "recv failed\n");
            break;
        }

        fprintf(stdout, "\n%s", buffer);
        fprintf(stdout, "> ");
        fflush(stdout);
    }

    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char message[BUFFER_SIZE];
    pthread_t recv_thread;

    int port = 1220;
    if (argc > 1) {
        port = atoi(argv[1]);
    }


    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Socket creation failed\n");
        return -1;
    }


    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address\n");
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Connection failed\n");
        close(sock);
        return -1;
    }


    fprintf(stdout, "Connected to server on port %d\n", port);
    fprintf(stdout, "Type messages or 'quit' to exit:\n");

    if (pthread_create(&recv_thread, NULL, receive_messages, (void*)&sock) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        close(sock);
        return -1;
    }


    while (1) {
        fprintf(stdout, "> ");
        fflush(stdout); 


        if (!fgets(message, sizeof(message), stdin)) break;

        if (strncmp(message, "quit", 4) == 0) break;

        if (send(sock, message, strlen(message), 0) == -1) {
            fprintf(stderr, "send failed\n");
            break;
        }
    }

    close(sock);
    pthread_cancel(recv_thread);
    fprintf(stdout, "Disconnected from server\n");
    return 0;
}


