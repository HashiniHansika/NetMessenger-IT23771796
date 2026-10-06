# NetMessenger – Multi-Client Chat and File-Sharing Platform

## Student Information

- Student ID: IT23771796
- Module: IE3010 Network Programming
- Project: NetMessenger
- Protocol: TCP/IP
- Language: C
- Platform: Linux
- Server Port: 7796
- NID: 7717

## Project Overview

NetMessenger is a TCP/IP-based multi-client messaging and file-sharing application developed in C using BSD sockets and POSIX threads.

The server supports multiple simultaneous clients and provides user registration, presence notifications, broadcast messaging, private messaging, chat rooms, file sharing, graceful disconnection, error handling, and server-side logging.

## Main Features

### 1. Multi-Client Support
The server uses POSIX threads to handle multiple clients concurrently. The implementation supports up to 10 connected clients.

### 2. User Registration
Clients register using:

REGISTER <username>

The server prevents duplicate usernames.

### 3. Presence
The server notifies connected clients when users join or leave.

### 4. Broadcast Messaging

BCAST <message>

The message is delivered to all other connected clients.

### 5. Private Messaging

PMSG <username> <message>

The server forwards a private message to the specified user.

### 6. Chat Rooms

JOIN <room>
LEAVE <room>
ROOMS
RMSG <room> <message>

Users can join rooms, leave rooms, view available rooms, and send room messages.

### 7. File Sharing

SENDFILE <target> <filename> <filesize>

The server receives the specified number of raw file bytes and stores the file under:

storage/IT23771796/<sender_username>/<filename>

The maximum supported file size is 10 MB.

### 8. Error Handling

The server provides protocol-level error responses for invalid commands, invalid messages, unavailable users, invalid files, and other invalid operations.

### 9. Logging

Server events are recorded in:

netmsg_IT23771796.log

Log entries include timestamps and important events such as connections, registrations, messaging, room activity, file transfers, and disconnections.

## Compilation

Compile the server:

gcc -Wall -Wextra -std=c11 -pthread server_1796.c -o server_1796

Compile the client:

gcc -Wall -Wextra -std=c11 -pthread client_1796.c -o client_1796

Or use the provided Makefile:

make -f Makefile_1796

## Running the Application

Start the server:

./server_1796

Start a client in another terminal:

./client_1796

The client connects to:

127.0.0.1:7796

## Testing

The implementation was tested with:

- Multiple simultaneous clients
- User registration
- Duplicate username handling
- Presence notifications
- Broadcast messaging
- Private messaging
- Room creation/joining and leaving
- Room messaging
- File transfer
- Invalid file handling
- File size validation
- Graceful disconnection
- Server logging
- Five simultaneous clients

## Storage

Received files are stored in:

storage/IT23771796/

Example:

storage/IT23771796/hashini/test.txt

## Source Files

- server_1796.c – Server implementation
- client_1796.c – Client implementation
- Makefile_1796 – Build configuration
