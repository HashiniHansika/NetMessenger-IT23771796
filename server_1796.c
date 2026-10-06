#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define PORT 7796
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define USERNAME_SIZE 50

#define NID "7717"
#define ROOM_NAME_SIZE 50

/* Information about a connected user */
typedef struct {
    int socket_fd;
    char username[USERNAME_SIZE];
    int registered;
    char room[ROOM_NAME_SIZE];
} client_t;

/* Shared client list */
client_t *clients[MAX_CLIENTS];

/* Mutex protects the shared client list */
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


/* ---------------------------------------------------------
   Send a complete response to a client
   --------------------------------------------------------- */
void send_response(int client_fd, const char *response)
{
    send(client_fd, response, strlen(response), MSG_NOSIGNAL);
}


/* ---------------------------------------------------------
   Check whether username already exists
   --------------------------------------------------------- */
int username_exists(const char *username)
{
    int exists = 0;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i] != NULL &&
            clients[i]->registered &&
            strcmp(clients[i]->username, username) == 0) {

            exists = 1;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    return exists;
}


/* ---------------------------------------------------------
   Add a client to the shared client list
   --------------------------------------------------------- */
int add_client(client_t *client)
{
    int added = 0;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i] == NULL) {

            clients[i] = client;
            added = 1;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    return added;
}


/* ---------------------------------------------------------
   Remove a client from the shared client list
   --------------------------------------------------------- */
void remove_client(client_t *client)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i] == client) {

            clients[i] = NULL;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}
/* ---------------------------------------------------------
   Notify all other registered users about presence changes
   --------------------------------------------------------- */
void broadcast_presence(const char *message,
                        client_t *sender)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i] != NULL &&
            clients[i]->registered &&
            clients[i] != sender) {

            send(clients[i]->socket_fd,
                 message,
                 strlen(message),
                 MSG_NOSIGNAL);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

/* ---------------------------------------------------------
   Display currently connected users
   --------------------------------------------------------- */
void print_connected_users(void)
{
    pthread_mutex_lock(&clients_mutex);

    printf("\n===== Connected Users =====\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {

        if (clients[i] != NULL &&
            clients[i]->registered) {

            printf("%d -> %s\n",
                   clients[i]->socket_fd,
                   clients[i]->username);
        }
    }

    printf("===========================\n");

    pthread_mutex_unlock(&clients_mutex);
}


/* ---------------------------------------------------------
   Handle one client
   --------------------------------------------------------- */
void *handle_client(void *arg)
{
    client_t *client = (client_t *)arg;

    int client_fd = client->socket_fd;

    char buffer[BUFFER_SIZE];

    printf("Client connected. Socket: %d\n",
           client_fd);


    while (1) {

        memset(buffer, 0, sizeof(buffer));

        int bytes_received =
            recv(client_fd,
                 buffer,
                 sizeof(buffer) - 1,
                 0);


        /* -------------------------------------------------
           Client sent data
           ------------------------------------------------- */
        if (bytes_received > 0) {

            buffer[bytes_received] = '\0';

            /* Remove newline if present */
            buffer[strcspn(buffer, "\r\n")] = '\0';

            printf("Client %d sent: %s\n",
                   client_fd,
                   buffer);


            /* =================================================
               REGISTER
               ================================================= */

            if (strncmp(buffer, "REGISTER ", 9) == 0) {

                char username[USERNAME_SIZE];

                memset(username, 0, sizeof(username));

                strncpy(username,
                        buffer + 9,
                        USERNAME_SIZE - 1);


                /* Check empty username */
                if (strlen(username) == 0) {

                    char response[BUFFER_SIZE];

                    snprintf(response,
                             sizeof(response),
                             "ERR 003 INVALID_USERNAME NID:%s\n",
                             NID);

                    send_response(client_fd,
                                  response);

                    continue;
                }


                /* Check whether username already exists */
                if (username_exists(username)) {

                    char response[BUFFER_SIZE];

                    snprintf(response,
                             sizeof(response),
                             "ERR 001 USERNAME_TAKEN NID:%s\n",
                             NID);

                    send_response(client_fd,
                                  response);

                    printf("Username '%s' already exists.\n",
                           username);

                    continue;
                }


                /* Add client to shared list */
                strncpy(client->username,
                        username,
                        USERNAME_SIZE - 1);

                client->username[USERNAME_SIZE - 1] = '\0';

                client->registered = 1;


                if (!add_client(client)) {

                    char response[BUFFER_SIZE];

                    snprintf(response,
                             sizeof(response),
                             "ERR 003 SERVER_FULL NID:%s\n",
                             NID);

                    send_response(client_fd,
                                  response);

                    client->registered = 0;

                    continue;
                }


                /* Send successful registration response */
                char response[BUFFER_SIZE];

                snprintf(response,
                         sizeof(response),
                         "OK REGISTERED %s NID:%s\n",
                         client->username,
                         NID);

                send_response(client_fd,
                              response);


               printf("User registered: %s\n",
       client->username);

/* Notify other connected users */
char presence_message[BUFFER_SIZE];

snprintf(presence_message,
         sizeof(presence_message),
         "MSG PRESENCE JOIN %s\n",
         client->username);

broadcast_presence(presence_message,
                   client);

print_connected_users();
            }
else if (strncmp(buffer, "BCAST ", 6) == 0)
{
    char *message = buffer + 6;

    if (strlen(message) == 0)
    {
        send_response(client->socket_fd,
                      "ERR 003 INVALID_MESSAGE NID:7717\n");
    }
    else
    {
        char msg_buffer[BUFFER_SIZE];

        snprintf(msg_buffer, sizeof(msg_buffer),
                 "MSG BCAST %s %s\n",
                 client->username, message);

        pthread_mutex_lock(&clients_mutex);

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i] != NULL &&
                clients[i]->registered &&
                clients[i] != client)
            {
                send(clients[i]->socket_fd,
                     msg_buffer,
                     strlen(msg_buffer),
                     MSG_NOSIGNAL);
            }
        }

        pthread_mutex_unlock(&clients_mutex);

        send_response(client->socket_fd,
                      "OK SENT NID:7717\n");
    }
}
else if (strncmp(buffer, "PMSG ", 5) == 0)
{
    char target_username[USERNAME_SIZE];
    char message[BUFFER_SIZE];

    char *space = strchr(buffer + 5, ' ');

    if (space == NULL)
    {
        send_response(client->socket_fd,
                      "ERR 003 INVALID_MESSAGE NID:7717\n");
    }
    else
    {
        size_t username_length = space - (buffer + 5);

        if (username_length == 0 ||
            username_length >= USERNAME_SIZE)
        {
            send_response(client->socket_fd,
                          "ERR 003 INVALID_USERNAME NID:7717\n");
        }
        else
        {
            strncpy(target_username, buffer + 5, username_length);
            target_username[username_length] = '\0';

            strcpy(message, space + 1);

            if (strlen(message) == 0)
            {
                send_response(client->socket_fd,
                              "ERR 003 INVALID_MESSAGE NID:7717\n");
            }
            else
            {
                int target_found = 0;

                char private_message[BUFFER_SIZE];

                snprintf(private_message,
                         sizeof(private_message),
                         "MSG PMSG %s %s\n",
                         client->username,
                         message);

                pthread_mutex_lock(&clients_mutex);

                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i] != NULL &&
                        clients[i]->registered &&
                        strcmp(clients[i]->username,
                               target_username) == 0)
                    {
                        send(clients[i]->socket_fd,
                             private_message,
                             strlen(private_message),
                             MSG_NOSIGNAL);

                        target_found = 1;
                        break;
                    }
                }

                pthread_mutex_unlock(&clients_mutex);

                if (target_found)
                {
                    send_response(client->socket_fd,
                                  "OK SENT NID:7717\n");
                }
                else
                {
                    send_response(client->socket_fd,
                                  "ERR 002 USER_NOT_FOUND NID:7717\n");
                }
            }
        }
    }
}
else if (strncmp(buffer, "JOIN ", 5) == 0)
{
    char *room_name = buffer + 5;

    if (strlen(room_name) == 0)
    {
        send_response(client->socket_fd,
                      "ERR 003 INVALID_ROOM NID:7717\n");
    }
    else if (strlen(room_name) >= ROOM_NAME_SIZE)
    {
        send_response(client->socket_fd,
                      "ERR 003 INVALID_ROOM NID:7717\n");
    }
    else
    {
        strncpy(client->room, room_name, ROOM_NAME_SIZE - 1);
        client->room[ROOM_NAME_SIZE - 1] = '\0';

        char response[BUFFER_SIZE];

        snprintf(response,
                 sizeof(response),
                 "OK JOINED %s NID:7717\n",
                 client->room);

        send_response(client->socket_fd, response);
    }
}
else if (strncmp(buffer, "LEAVE ", 6) == 0)
{
    char *room_name = buffer + 6;

    if (strlen(room_name) == 0)
    {
        send_response(client->socket_fd,
                      "ERR 003 INVALID_ROOM NID:7717\n");
    }
    else if (strlen(room_name) >= ROOM_NAME_SIZE)
    {
        send_response(client->socket_fd,
                      "ERR 003 INVALID_ROOM NID:7717\n");
    }
    else if (strcmp(client->room, room_name) != 0)
    {
        send_response(client->socket_fd,
                      "ERR 003 NOT_IN_ROOM NID:7717\n");
    }
    else
    {
        char response[BUFFER_SIZE];

        snprintf(response,
                 sizeof(response),
                 "OK LEFT %s NID:7717\n",
                 client->room);

        client->room[0] = '\0';

        send_response(client->socket_fd, response);
    }
}
else if (strcmp(buffer, "ROOMS") == 0)
{
    char rooms[BUFFER_SIZE] = "";
    int first_room = 1;

    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clients[i] != NULL &&
            clients[i]->registered &&
            clients[i]->room[0] != '\0')
        {
            int already_added = 0;

            char temp_rooms[BUFFER_SIZE];
            strncpy(temp_rooms, rooms, sizeof(temp_rooms) - 1);
            temp_rooms[sizeof(temp_rooms) - 1] = '\0';

            char *token = strtok(temp_rooms, ",");

            while (token != NULL)
            {
                if (strcmp(token, clients[i]->room) == 0)
                {
                    already_added = 1;
                    break;
                }

                token = strtok(NULL, ",");
            }

            if (!already_added)
            {
                if (!first_room)
                {
                    strncat(rooms, ",", sizeof(rooms) - strlen(rooms) - 1);
                }

                strncat(rooms,
                        clients[i]->room,
                        sizeof(rooms) - strlen(rooms) - 1);

                first_room = 0;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);

    char response[BUFFER_SIZE];

    if (strlen(rooms) > 0)
    {
        snprintf(response,
                 sizeof(response),
                 "OK ROOMS %.980s NID:7717\n",
                 rooms);
    }
    else
    {
        snprintf(response,
                 sizeof(response),
                 "OK ROOMS NID:7717\n");
    }

    send_response(client->socket_fd, response);
}
else if (strncmp(buffer, "RMSG ", 5) == 0)
{
    char *space = strchr(buffer + 5, ' ');

    if (space == NULL)
    {
        send_response(client->socket_fd,
                      "ERR 003 INVALID_MESSAGE NID:7717\n");
    }
    else
    {
        char room_name[ROOM_NAME_SIZE];
        char *message = space + 1;

        size_t room_length = space - (buffer + 5);

        if (room_length == 0 ||
            room_length >= ROOM_NAME_SIZE ||
            strlen(message) == 0)
        {
            send_response(client->socket_fd,
                          "ERR 003 INVALID_MESSAGE NID:7717\n");
        }
        else
        {
            strncpy(room_name, buffer + 5, room_length);
            room_name[room_length] = '\0';

            if (strcmp(client->room, room_name) != 0)
            {
                send_response(client->socket_fd,
                              "ERR 003 NOT_IN_ROOM NID:7717\n");
            }
            else
            {
                char room_message[BUFFER_SIZE];

                snprintf(room_message,
                         sizeof(room_message),
                         "MSG ROOM %s %s %s\n",
                         room_name,
                         client->username,
                         message);

                int sent_to_someone = 0;

                pthread_mutex_lock(&clients_mutex);

                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i] != NULL &&
                        clients[i]->registered &&
                        clients[i] != client &&
                        strcmp(clients[i]->room, room_name) == 0)
                    {
                        send(clients[i]->socket_fd,
                             room_message,
                             strlen(room_message),
                             MSG_NOSIGNAL);

                        sent_to_someone = 1;
                    }
                }

                pthread_mutex_unlock(&clients_mutex);

                send_response(client->socket_fd,
                              "OK SENT NID:7717\n");

                (void)sent_to_someone;
            }
        }
    }
}

            /* =================================================
               LIST
               ================================================= */

            else if (strcmp(buffer, "LIST") == 0) {

                char users[BUFFER_SIZE];

                memset(users, 0, sizeof(users));

                int first = 1;


                pthread_mutex_lock(&clients_mutex);

                for (int i = 0; i < MAX_CLIENTS; i++) {

                    if (clients[i] != NULL &&
                        clients[i]->registered) {

                        if (!first) {
                            strncat(users,
                                    ",",
                                    sizeof(users) -
                                    strlen(users) - 1);
                        }

                        strncat(users,
                                clients[i]->username,
                                sizeof(users) -
                                strlen(users) - 1);

                        first = 0;
                    }
                }

                pthread_mutex_unlock(&clients_mutex);


                char response[BUFFER_SIZE];

               if (strlen(users) > 0) {

    snprintf(response,
             sizeof(response),
             "OK USERS %.980s NID:%s\n",
             users,
             NID);

} else {

    snprintf(response,
             sizeof(response),
             "OK USERS NID:%s\n",
             NID);
}

                send_response(client_fd,
                              response);
            }


            /* =================================================
               QUIT
               ================================================= */

            else if (strcmp(buffer, "QUIT") == 0) {

                char response[BUFFER_SIZE];

                snprintf(response,
                         sizeof(response),
                         "OK BYE NID:%s\n",
                         NID);

                send_response(client_fd,
                              response);

                printf("User requested disconnect: %s\n",
                       client->registered ?
                       client->username : "unregistered");

                break;
            }


            /* =================================================
               Unknown command
               ================================================= */

            else {

                char response[BUFFER_SIZE];

                snprintf(response,
                         sizeof(response),
                         "ERR 003 UNKNOWN_COMMAND NID:%s\n",
                         NID);

                send_response(client_fd,
                              response);
            }
        }


        /* -------------------------------------------------
           Client disconnected normally
           ------------------------------------------------- */
        else if (bytes_received == 0) {

            printf("Client %d disconnected.\n",
                   client_fd);

            break;
        }


        /* -------------------------------------------------
           Receive error
           ------------------------------------------------- */
        else {

            perror("recv");
            break;
        }
    }


    /* Remove registered client from shared list */
    if (client->registered) {

    printf("Removing user: %s\n",
           client->username);

    /* Notify other users before removing the client */
    char presence_message[BUFFER_SIZE];

    snprintf(presence_message,
             sizeof(presence_message),
             "MSG PRESENCE LEAVE %s\n",
             client->username);

    broadcast_presence(presence_message,
                       client);

    remove_client(client);
}


    close(client_fd);

    free(client);

    return NULL;
}


/* ---------------------------------------------------------
   MAIN
   --------------------------------------------------------- */
int main(void)
{
    int server_fd;

    struct sockaddr_in server_addr;


    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);


    /* Initialize client list */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = NULL;
    }


    /* -----------------------------------------------------
       1. Create socket
       ----------------------------------------------------- */

    server_fd =
        socket(AF_INET,
               SOCK_STREAM,
               0);

    if (server_fd < 0) {

        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket created successfully.\n");


    /* -----------------------------------------------------
       2. Allow address reuse
       ----------------------------------------------------- */

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


    /* -----------------------------------------------------
       3. Configure server address
       ----------------------------------------------------- */

    memset(&server_addr,
           0,
           sizeof(server_addr));

    server_addr.sin_family = AF_INET;

    server_addr.sin_addr.s_addr =
        INADDR_ANY;

    server_addr.sin_port =
        htons(PORT);


    /* -----------------------------------------------------
       4. Bind
       ----------------------------------------------------- */

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {

        perror("bind");

        close(server_fd);

        exit(EXIT_FAILURE);
    }

    printf("Server bound to port %d.\n",
           PORT);


    /* -----------------------------------------------------
       5. Listen
       ----------------------------------------------------- */

    if (listen(server_fd,
               MAX_CLIENTS) < 0) {

        perror("listen");

        close(server_fd);

        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");

    printf("Maximum clients: %d\n",
           MAX_CLIENTS);


    /* -----------------------------------------------------
       6. Accept clients
       ----------------------------------------------------- */

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


        /* Allocate client structure */
        client_t *client =
            malloc(sizeof(client_t));


        if (client == NULL) {

            perror("malloc");

            close(client_fd);

            continue;
        }


        memset(client,
               0,
               sizeof(client_t));


        client->socket_fd =
            client_fd;

        client->registered = 0;


        /* Create client thread */
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


        pthread_detach(thread_id);


        printf("New client thread created.\n");
    }


    close(server_fd);

    return 0;
}
