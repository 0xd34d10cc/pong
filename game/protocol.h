#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>

#define MAX_MESSAGE_SIZE 256
#define MAX_PASSWORD_SIZE 32


typedef enum {
  ERROR_STATUS = 0xff,

  // client messages
  CREATE_LOBBY = 0x0,
  JOIN_LOBBY = 0x1,

  // server messages
  LOBBY_CREATED = 0x2,
  LOBBY_JOINED = 0x3,
} MessageType;


// Create a new game lobby
typedef struct {
  char password[MAX_PASSWORD_SIZE];
} CreateLobby;

// Sent in response to CreateSession message
typedef struct {
  // Identifier of the lobby
  int id;
} LobbyCreated;

// Join existing lobby
typedef struct {
  int id;
  char password[MAX_PASSWORD_SIZE];
} JoinLobby;

// Sent in response to JoinLobby message
// and also to the owner of game lobby when someone joins
typedef struct {
  // ip address of opponent
  char ipv4[16];
} LobbyJoined;

// Error statuses
enum {
  // There are already 2 players in this session
  LOBBY_IS_FULL,

  // Provided lobby id is invalid
  INVALID_LOBBY_ID,

  // Provided password is invalid
  INVALID_PASSWORD,

  // Opponent disconnected unexpectedly
  OPPONENT_DISCONNECTED,

  // internal error means that something went wrong inside server
  INTERNAL_ERROR,

  ERROR_STATUS_MAX
};

// Message sent in case of error
typedef struct {
  int status;
} ErrorStatus;

// Client message structure
typedef struct {
  unsigned short id;
  union {
    CreateLobby create_lobby;
    JoinLobby join_lobby;
  };
} ClientMessage;

// Server message structure
typedef struct ServerMsg {
  unsigned short id;
  union {
    LobbyCreated lobby_created;
    LobbyJoined lobby_joined;
    ErrorStatus error;
  };
} ServerMessage;

// Read message from the buffer
// returns:
// -1 i   n case of protocol violation
// 0      if there is not enough data in buffer
// n > 0  on success, where n is the number of bytes read from buffer
int client_message_read(ClientMessage* message, const char* buffer, size_t size);
int server_message_read(ServerMessage* message, const char* buffer, size_t size);

// Write message to the buffer
// returns:
// 0      if there is not enough space in buffer
// n > 0  on success, where n is the number of bytes written to buffer
int client_message_write(const ClientMessage* message, char* buffer, size_t size);
int server_message_write(const ServerMessage* message, char* buffer, size_t size);

#endif // MESSAGES_H
