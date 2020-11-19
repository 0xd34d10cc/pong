#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>

#include "config.h"

// Status that we provide to end users about connection success or failure (SessionJoined message)
typedef enum {
  // Successfully joined game session
  JOINED = 0x0,

  // Provided wrong session ID
  INVALID_SESSION_ID = 0x1,

  // Provided wrong password for existing session
  INVALID_PASSWORD = 0x2,

  // internal error means that something went wrong inside server
  INTERNAL_ERROR = 0x3,

  SESSION_JOIN_STATUS_MAX = 0x4
} SessionJoinStatus;

typedef enum {
  // client messages
  CREATE_SESSION = 0x0,
  JOIN_SESSION = 0x1,

  // server messages
  SESSION_CREATED = 0x2,
  SESSION_JOINED = 0x3,
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
