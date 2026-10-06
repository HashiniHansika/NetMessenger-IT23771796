#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 7796
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    int socket_fd;
    struct sockaddr_in address;
} client_info_t;

/* Handles one connected client */
void *handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;

    int client_fd = client->socket_fd;
    char buffer[BUFFER_SIZE];

    printf("Client connected. Socket: %d\n", client_fd);

    while (1) {

        memset(buffer, 0, sizeof(buffer));

        int bytes_received = recv(client_fd,
                                  buffer,
                                  sizeof(buffer) - 1,
                                  0);

        if (bytes_received > 0) {

            buffer[bytes_received] = '\0';

            printf("Client %d sent: %s\n",
                   client_fd,
                   buffer);

            /* Temporary REGISTER handling */
            if (strncmp(buffer, "REGISTER ", 9) == 0) {

                char *username = buffer + 9;

                printf("Registration request from: %s\n",
                       username);

                const char *response = "REGISTERED";

                send(client_fd,
                     response,
                     strlen(response),
                     0);

                printf("Response sent to client %d: %s\n",
                       client_fd,
                       response);
            }
            else {

                const char *response = "ERROR Unknown command";

                send(client_fd,
                     response,
                     strlen(response),
                     0);
            }
        }
        else if (bytes_received == 0) {

            printf("Client %d disconnected.\n",
                   client_fd);

            break;
        }
        else {

            perror("recv");
            break;
        }
    }

    close(client_fd);

    free(client);

    return NULL;
}

int main()
{
    int server_fd;

    struct sockaddr_in server_addr;

    /* 1. Create server socket */
    server_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);

    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket created successfully.\n");

    /* Allow reuse of the port */
    int opt = 1;

    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) < 0) {

        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* 2. Configure server address */
    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* 3. Bind */
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server bound to port %d.\n",
           PORT);

    /* 4. Listen */
    if (listen(server_fd, MAX_CLIENTS) < 0) {

        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");
    printf("Maximum clients: %d\n",
           MAX_CLIENTS);

    /* 5. Accept clients continuously */
    while (1) {

        struct sockaddr_in client_addr;

        socklen_t client_len =
            sizeof(client_addr);

        int client_fd =
            accept(server_fd,
                   (struct sockaddr *)&client_addr,
                   &client_len);

        if (client_fd < 0) {

            perror("accept");
            continue;
        }

        /* Allocate information for the new client */
        client_info_t *client =
            malloc(sizeof(client_info_t));

        if (client == NULL) {

            perror("malloc");
            close(client_fd);
            continue;
        }

        client->socket_fd = client_fd;
        client->address = client_addr;

        /* Create a thread for this client */
        pthread_t thread_id;

        if (pthread_create(&thread_id,
                           NULL,
                           handle_client,
                           client) != 0) {

            perror("pthread_create");

            close(client_fd);
            free(client);

            continue;
        }

        /*
         * The server does not need to wait for
         * this client thread.
         */
        pthread_detach(thread_id);

        printf("New client thread created.\n");
    }

    close(server_fd);

    return 0;
}
