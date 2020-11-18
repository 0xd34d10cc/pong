#ifndef MESSAGES_H
#define MESSAGES_H

#define PWDEFAULTSIZE 100

enum MessageType {
  CREATE_GAME_SESSION = 0,
  SEND_SESSION,
  CONNECT_TO_SESSION,
  SEND_STATUS,
  NOTIFY_USER,

  INVALID_TYPE = -1
};

enum MessageType getMessageType(char* buffer);

struct CreateGameSessionMsg {
  int pw_size;
  char pw[PWDEFAULTSIZE];
};

struct SendSessionMsg {
  short id;
  int session_id;
};

struct ConnectToSessionMsg {
  short id;
  int session_id;
  int pw_size;
  char pw[PWDEFAULTSIZE];
};

struct SendStatusMsg {
  short id;
  int status_code;
};

struct NotifyUserMsg {
  short id;
  char ipv4[16];
  int status_code;
};

#endif // MESSAGES_H
