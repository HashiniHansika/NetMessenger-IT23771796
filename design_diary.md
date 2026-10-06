# NetMessenger Design Diary

## Student ID
IT23771796

## Project
NetMessenger – Multi-Client Chat and File-Sharing Platform

## Development Approach

I developed NetMessenger incrementally using C, BSD sockets, TCP/IP, and POSIX threads on Linux. The implementation was developed and tested feature by feature rather than implementing the complete system at once.

## Development Stages

### Stage 1 – Basic TCP Server and Client
I first created a TCP server that creates a socket, binds it to the personalized port 7796, listens for connections, and accepts clients. A basic client was created to connect to the server.

### Stage 2 – Multi-Client Concurrency
POSIX threads were introduced so that each connected client could be handled independently. A shared client list protected by a mutex was used to manage concurrent access.

### Stage 3 – Registration and Presence
User registration was implemented using the REGISTER command. Unique usernames are maintained by the server, and presence notifications are sent when users connect or disconnect.

### Stage 4 – Messaging
Broadcast messaging and private messaging were implemented using the BCAST and PMSG commands. The server validates the target user for private messages.

### Stage 5 – Chat Rooms
Room functionality was added using JOIN, LEAVE, ROOMS, and RMSG commands. Each client maintains its current room and room messages are delivered only to users in the same room.

### Stage 6 – File Sharing
File transfer was implemented using the SENDFILE command. The client sends the command followed by the exact number of raw file bytes specified by the filesize field. The server receives the exact byte count and stores the file under the personalized storage directory.

A maximum file size of 10 MB was introduced to prevent excessively large transfers.

### Stage 7 – Robust Protocol Handling
The server was improved to process TCP input correctly. A line-based receiving function was implemented to handle partial lines and multiple commands safely. File data is handled separately using the exact filesize value.

### Stage 8 – Error Handling
Validation was added for malformed commands, unavailable users, invalid rooms, invalid files, oversized files, and other invalid requests.

### Stage 9 – Logging
Timestamped server logging was implemented using the personalized log file:

netmsg_IT23771796.log

Important events such as client connections, registrations, messages, room activity, file transfers, and disconnections are recorded.

## Concurrency Design

The server uses one POSIX thread per connected client. Shared client information is protected using a pthread mutex. This allows multiple clients to communicate concurrently while reducing race conditions when accessing the shared client list.

## Testing

The application was tested using multiple simultaneous clients. Testing covered registration, presence notifications, broadcast messaging, private messaging, room messaging, file transfers, invalid inputs, graceful disconnection, logging, and five simultaneous clients.

## Problems and Improvements

During development, TCP stream handling required special attention because TCP does not preserve application-level message boundaries. The server was therefore changed from a simple recv-based command approach to a line-based receiver for text commands and an exact-byte receiver for file transfers.

A compiler warning related to potentially long private messages was also resolved by limiting the message length when constructing the outgoing PMSG response.

## Final Result

The final implementation provides a functional multi-client TCP chat and file-sharing platform with personalized identifiers, concurrent client handling, messaging, rooms, file transfer, error handling, and timestamped logging.

