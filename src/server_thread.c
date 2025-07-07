#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>


#define PORT 1221
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    int socket;
    struct sockaddr_in addr;
    int id;
    int slot;
} client_info_t;


static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static client_info_t* clients[MAX_CLIENTS];
static int client_count = 0;
static volatile int server_running = 1;


static void cleanup_client(client_info_t* client);
static int add_client(client_info_t* client);
static void remove_client(client_info_t* client);
static void broadcast_message(const char*msg, int sender_id);


void* handle_client(void* arg) {
    client_info_t* client = (client_info_t*)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;


    if (!client) {
        fprintf(stderr, "handle_client: NULL client pointer\n");
        pthread_exit(NULL);
    }


    fprintf(stdout, "Client %d connected from %s:%d\n", client -> id,
        inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));


    while (server_running) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = recv(client->socket, buffer, BUFFER_SIZE - 1, 0);


        if (bytes_read <= 0) {

            if (bytes_read == 0) fprintf(stdout, "Client %d disconnected\n", client->id);
            else if (errno != EINTR) fprintf(stderr, "Client %d recv error: %s\n", client->id, strerror(errno));
            break;
        }

        fprintf(stdout, "Client %d: %s\n", client->id, buffer);


        char broadcast_msg[BUFFER_SIZE + 50];
        int msg_len = snprintf(broadcast_msg, sizeof(broadcast_msg),
                               "Client %d: %s\n", client->id, buffer);


        if (msg_len > 0 && msg_len < (int)sizeof(broadcast_msg))
            broadcast_message(broadcast_msg, client->id);


        char ack[BUFFER_SIZE + 20];
        int ack_len = snprintf(ack, sizeof(ack), "Echo: %s\n", buffer);

        if (ack_len > 0 && ack_len < (int)sizeof(ack)){
            if (send(client->socket, ack, ack_len, MSG_NOSIGNAL) == -1) {
                if (errno != EPIPE && errno != ECONNRESET)
                    fprintf(stderr, "Client %d send error: %s\n", client->id, strerror(errno));
                break;
            }
        }
    }

    cleanup_client(client);
    pthread_exit(NULL);

}


static void cleanup_client(client_info_t* client) {
    if (!client) return;

    fprintf(stdout, "Cleaning up client %d\n", client->id);
    remove_client(client);
    close(client->socket);
    free(client);
}


static int add_client(client_info_t* client) {
    if (!client) return -1;

    pthread_mutex_lock(&clients_mutex);

    if (client_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clients_mutex);
        return -1;
    }


    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = client;
            client->slot = i;
            client_count++;
            pthread_mutex_unlock(&clients_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
    return -1;
}



static void remove_client(client_info_t* client) {
    if (!client) return;

    pthread_mutex_lock(&clients_mutex);

    if (client->slot >= 0 && client->slot < MAX_CLIENTS &&
        clients[client->slot] == client) {
        clients[client->slot] = NULL;
        client_count--;
    }

    pthread_mutex_unlock(&clients_mutex);
}


static void broadcast_message(const char* msg, int sender_id) {
    if (!msg) return;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL && clients[i]->id != sender_id) {
            if (send(clients[i]->socket, msg, strlen(msg), MSG_NOSIGNAL) == -1) {
                if (errno != EPIPE && errno != ECONNRESET) 
                    fprintf(stderr, "Broadcast to client %d failed: %s\n",
                            clients[i]->id, strerror(errno));
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}


static void signal_handler(int sig) {
    fprintf(stdout, "Shutting down server (signal %d)\n", sig);
    server_running = 0;
}


int main() {
    int server_fd, client_fd;
    struct sockaddr_in addr;
    int opt = 1;
    socklen_t addrlen = sizeof(addr);
    pthread_t thread_id;
    int client_id = 1;

    memset(clients, 0, sizeof(clients));
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        fprintf(stderr, "socket creation failed\n");
        exit(EXIT_FAILURE);
    }


    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        fprintf(stderr, "setsockopt failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, addrlen);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        fprintf(stderr, "bind failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) {
        fprintf(stderr, "listen failed\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "threaded server listening on port %d\n", PORT);
    fprintf(stdout, "Press Ctrl+C to shutdown\n");

    while (server_running) {
        client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_fd == -1) {
            if (errno == EINTR) continue;
            fprintf(stderr, "accept failed\n");
            continue;
        }

        client_info_t* client = malloc(sizeof(client_info_t));
 
        if (!client) {
            fprintf(stderr, "client malloc failed\n");
            close(client_fd);
            continue;
        }

        client -> socket = client_fd;
        client -> addr = addr;
        client -> id = client_id++;
        client -> slot = -1;

        if (add_client(client) != 0) {
            fprintf(stderr, "Max clients reached or add failed\n");
            remove_client(client);
            close(client_fd);
            free(client);
            continue;
        }


        int thread_result = pthread_create(&thread_id, NULL, handle_client, client);
        if (thread_result != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(thread_result));
            remove_client(client); 
            close(client_fd); 
            free(client); 
            continue;
        }

        pthread_detach(thread_id);

    }

    fprintf(stdout, "Shutting down server...\n");

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL) close(clients[i]->socket);
    }


    pthread_mutex_unlock(&clients_mutex);

    close(server_fd);
    fprintf(stdout, "Server shutdown complete\n");
    return 0;
}





    
