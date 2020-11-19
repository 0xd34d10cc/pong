#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>

#include "config.h"

// Status that we provide to end users about connection success or failure (SessionJoined message)
typedef enum {
  // Successfully connected to the game session
  CONNECTED,
  // Provided wrong session ID
  WRONG_SESSION_ID,
  // Provided wrong password for existing session
  WRONG_PASSWORD,

  // internal error means that something went wrong inside server
  INTERNAL_ERROR,

  SESSION_JOINED_STATUS_MAX
} SessionJoinStatus;

typedef enum {
  // client messages
  CREATE_SESSION,
  JOIN_SESSION,

  // server messages
  SESSION_CREATED,
  SESSION_JOINED,
} MessageType;

// Create a new game session
typedef struct {
  char password[MAX_PASSWORD_SIZE];
} CreateSession;

// Sent in response to CreateSession message
typedef struct {
  int session_id;
} SessionCreated;

// Join existing game session
typedef struct {
  int session_id;
  char password[MAX_PASSWORD_SIZE];
} JoinSession;

// Sent in response to JoinSession message
// and also to the owner of session
typedef struct {
  int status_code;
  char ipv4[16];
} SessionJoined;

// Client message structure
typedef struct {
  unsigned short id;
  union {
    CreateSession create_session;
    JoinSession join_session;
  };
} ClientMessage;


// Server message structure
typedef struct ServerMsg {
  unsigned short id;
  union {
    SessionCreated session_created;
    SessionJoined session_joined;
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
