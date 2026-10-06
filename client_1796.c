#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_PORT 7796
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // 1. Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Client socket created.\n");

    // 2. Configure server address
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, "127.0.0.1",
                  &server_addr.sin_addr) <= 0) {

        perror("inet_pton");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    // 3. Connect to server
    if (connect(sock_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {

        perror("connect");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

   // 4. Send REGISTER command
strcpy(buffer, "REGISTER hashini");

send(sock_fd, buffer, strlen(buffer), 0);

printf("Message sent: %s\n", buffer);

// 5. Receive server response
memset(buffer, 0, sizeof(buffer));

int bytes_received = recv(sock_fd,
                          buffer,
                          sizeof(buffer) - 1,
                          0);

if (bytes_received > 0) {
    buffer[bytes_received] = '\0';

    printf("Server response: %s\n", buffer);
}

    // 5. Close socket
    close(sock_fd);

    return 0;
}
