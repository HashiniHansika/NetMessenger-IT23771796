#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#define USERNAME_SIZE 50
#define MAX_FILE_SIZE (10 * 1024 * 1024)
#define PORT 7796
#define BUFFER_SIZE 1024

volatile int running = 1;

/* ---------------------------------------------------------
   Receiver thread
   Continuously waits for messages from the server
   --------------------------------------------------------- */
void *receive_messages(void *arg)
{
    int client_fd = *(int *)arg;

    char buffer[BUFFER_SIZE];

    while (running) {

        memset(buffer, 0, sizeof(buffer));

        int bytes_received =
            recv(client_fd,
                 buffer,
                 sizeof(buffer) - 1,
                 0);

        if (bytes_received > 0) {

            buffer[bytes_received] = '\0';

            printf("\n%s", buffer);

            printf("> ");
            fflush(stdout);
        }

        else if (bytes_received == 0) {

            printf("\nServer disconnected.\n");

            running = 0;

            break;
        }

        else {

            if (running) {
                perror("recv");
            }

            running = 0;

            break;
        }
    }

    return NULL;
}


/* ---------------------------------------------------------
   MAIN
   --------------------------------------------------------- */
int main(void)
{
    int client_fd;

    struct sockaddr_in server_addr;

    char buffer[BUFFER_SIZE];

    char username[100];

    pthread_t receiver_thread;


    /* -----------------------------------------------------
       1. Create socket
       ----------------------------------------------------- */

    client_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if (client_fd < 0) {

        perror("socket");

        exit(EXIT_FAILURE);
    }

    printf("Client socket created.\n");


    /* -----------------------------------------------------
       2. Configure server address
       ----------------------------------------------------- */

    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family =
        AF_INET;

    server_addr.sin_port =
        htons(PORT);

    server_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");


    /* -----------------------------------------------------
       3. Connect
       ----------------------------------------------------- */

    if (connect(client_fd,
                (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {

        perror("connect");

        close(client_fd);

        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");


    /* -----------------------------------------------------
       4. Ask for username
       ----------------------------------------------------- */

    printf("Enter username: ");
    fflush(stdout);

    if (fgets(username,
              sizeof(username),
              stdin) == NULL) {

        close(client_fd);

        return 0;
    }


    username[strcspn(username,
                     "\r\n")] = '\0';


    /* -----------------------------------------------------
       5. Send REGISTER
       ----------------------------------------------------- */

    snprintf(buffer,
             sizeof(buffer),
             "REGISTER %s\n",
             username);

    send(client_fd,
         buffer,
         strlen(buffer),
         0);

    printf("Message sent: %s",
           buffer);


    /* -----------------------------------------------------
       6. Receive registration response
       ----------------------------------------------------- */

    memset(buffer,
           0,
           sizeof(buffer));

    int bytes_received =
        recv(client_fd,
             buffer,
             sizeof(buffer) - 1,
             0);

    if (bytes_received <= 0) {

        printf("Server disconnected.\n");

        close(client_fd);

        return 0;
    }

    buffer[bytes_received] = '\0';

    printf("Server response: %s",
           buffer);


    /* -----------------------------------------------------
       7. Check registration result
       ----------------------------------------------------- */

    if (strncmp(buffer,
                "OK REGISTERED",
                13) != 0) {

        printf("Registration failed.\n");

        close(client_fd);

        return 0;
    }


    printf("\nYou are now connected.\n");
    printf("Type a command and press Enter.\n");
    printf("Type QUIT to disconnect.\n\n");


    /* -----------------------------------------------------
       8. Start receiver thread
       ----------------------------------------------------- */

    if (pthread_create(&receiver_thread,
                       NULL,
                       receive_messages,
                       &client_fd) != 0) {

        perror("pthread_create");

        close(client_fd);

        return 0;
    }


    /* -----------------------------------------------------
       9. Main input loop
       ----------------------------------------------------- */

    while (running) {

        printf("> ");
        fflush(stdout);


        if (fgets(buffer,
                  sizeof(buffer),
                  stdin) == NULL) {

            break;
        }
/* -------------------------------------------------
   SENDFILE command
   Format:
   SENDFILE <target> <filename> <filesize>
   ------------------------------------------------- */
if (strncmp(buffer, "SENDFILE ", 9) == 0)
{
    char target[USERNAME_SIZE];
    char filename[256];
    long filesize;

    int parsed = sscanf(buffer + 9,
                        "%49s %255s %ld",
                        target,
                        filename,
                        &filesize);

    if (parsed != 3 || filesize <= 0)
    {
        printf("Invalid SENDFILE format.\n");
        continue;
    }

    if (filesize > MAX_FILE_SIZE)
    {
        printf("File is too large. Maximum size is 10 MB.\n");
        continue;
    }

    int file_fd = open(filename, O_RDONLY);

    if (file_fd < 0)
    {
        perror("open");
        continue;
    }

    struct stat file_info;

    if (fstat(file_fd, &file_info) < 0)
    {
        perror("fstat");
        close(file_fd);
        continue;
    }

    if (file_info.st_size != filesize)
    {
        printf("Specified filesize does not match actual file size.\n");
        close(file_fd);
        continue;
    }

    /* Send the SENDFILE command first */
    if (send(client_fd,
             buffer,
             strlen(buffer),
             MSG_NOSIGNAL) < 0)
    {
        perror("send");
        close(file_fd);
        break;
    }

    /* Send exactly filesize bytes */
    long total_sent = 0;

    while (total_sent < filesize)
    {
        char file_buffer[4096];

        ssize_t bytes_read =
            read(file_fd,
                 file_buffer,
                 sizeof(file_buffer));

        if (bytes_read <= 0)
        {
            perror("read");
            break;
        }

        ssize_t offset = 0;

        while (offset < bytes_read)
        {
            ssize_t bytes_sent =
                send(client_fd,
                     file_buffer + offset,
                     bytes_read - offset,
                     MSG_NOSIGNAL);

            if (bytes_sent <= 0)
            {
                perror("send");
                break;
            }

            offset += bytes_sent;
            total_sent += bytes_sent;
        }
    }

    close(file_fd);

    if (total_sent == filesize)
    {
        printf("File sent successfully: %s (%ld bytes)\n",
               filename,
               filesize);
    }

    continue;
}


        /* Send command to server */

        if (send(client_fd,
                 buffer,
                 strlen(buffer),
                 MSG_NOSIGNAL) < 0) {

            perror("send");

            break;
        }


        /* QUIT */
        if (strncmp(buffer,
                    "QUIT",
                    4) == 0) {

            /*
             * Give the receiver thread a moment
             * to receive OK BYE.
             */
            sleep(1);

            running = 0;

            break;
        }
    }


    /* -----------------------------------------------------
       10. Stop receiver
       ----------------------------------------------------- */

    shutdown(client_fd,
             SHUT_RDWR);

    pthread_join(receiver_thread,
                 NULL);


    /* -----------------------------------------------------
       11. Close socket
       ----------------------------------------------------- */

    close(client_fd);

    printf("Disconnected from server.\n");

    return 0;
}
