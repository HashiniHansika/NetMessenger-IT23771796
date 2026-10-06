#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 7796
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    char buffer[BUFFER_SIZE];

    // 1. Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket created successfully.\n");

    // 2. Configure server address
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 3. Bind socket to port
    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server bound to port %d.\n", PORT);

    // 4. Start listening
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");

    // 5. Accept one client
    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &client_len);

    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client connected.\n");

    // 6. Receive command
    memset(buffer, 0, sizeof(buffer));

    int bytes_received = recv(client_fd,
                              buffer,
                              sizeof(buffer) - 1,
                              0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        printf("Received: %s\n", buffer);

        // 7. Process REGISTER command
        if (strncmp(buffer, "REGISTER ", 9) == 0) {

            char *username = buffer + 9;

            printf("Registration request for user: %s\n", username);

            const char *response = "REGISTERED";

            send(client_fd,
                 response,
                 strlen(response),
                 0);

            printf("Response sent: %s\n", response);
        }
        else {
            const char *response = "ERROR Unknown command";

            send(client_fd,
                 response,
                 strlen(response),
                 0);

            printf("Response sent: %s\n", response);
        }
    }

    // 8. Close connection
    close(client_fd);
    close(server_fd);

    return 0;
}
