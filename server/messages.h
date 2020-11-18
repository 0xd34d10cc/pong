#ifndef MESSAGES_H
#define MESSAGES_H

#include <stddef.h>

#define PWDEFAULTSIZE 100

// Status that we provide to end users about connection success or failure (SendStatus msg)
typedef enum ClientStatus {
  // Successfully connected to the game session
  CONNECTED = 0,
  // Provided Wrong session ID
  WRONG_SESSION_ID = 1,
  // Provided Wrong password for existing session
  WRONG_PASSWORD = 2,

  // internal error means that something went wrong inside server
  INTERNAL_ERROR = 3,

  CLIENT_STATUS_MAX = 4
} ClientStatus;


enum MessageType {
  // client messages
  CREATE_GAME_SESSION = 0,
  CONNECT_TO_SESSION,

  // server messages
  SEND_SESSION,
  SEND_STATUS,
  NOTIFY_USER
};

typedef struct CreateGameSessionMsg {
  int pw_size;
  char pw[PWDEFAULTSIZE];
} CreateGameSessionMsg;

typedef struct SendSessionMsg {
  int session_id;
} SendSessionMsg;

typedef struct ConnectToSessionMsg {
  int session_id;
  int pw_size;
  char pw[PWDEFAULTSIZE];
} ConnectToSessionMsg;

typedef struct SendStatusMsg {
  int status_code;
} SendStatusMsg;

typedef struct NotifyUserMsg {
  char ipv4[16];
  int status_code;
} NotifyUserMsg;

typedef struct ClientMsg {
  short id;
  union {
    CreateGameSessionMsg create_game_session;
    ConnectToSessionMsg connect_to_session;
  };
} ClientMsg ;

typedef struct ServerMsg {
  short id;
  union {
    SendSessionMsg send_session;
    SendStatusMsg send_status;
    NotifyUserMsg notify_user;
  };
} ServerMsg;

int parse_server_message(struct ServerMsg* msg, const char* buffer, size_t size);
int parse_client_message(struct ClientMsg* msg, const char* buffer, size_t size);

int fill_server_message(const struct ServerMsg* msg, char* buffer, size_t size);
int fill_client_message(const struct ClientMsg* msg, char* buffer, size_t size);


#endif // MESSAGES_H
