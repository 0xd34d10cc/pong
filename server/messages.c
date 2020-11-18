#include "messages.h"

#include <stdlib.h>
#include <string.h>

#include "log.h"

#define NOT_ENOUGH_DATA 0
#define PARSE_ERROR -1

static int parse_create_session(struct CreateGameSessionMsg* msg, const char* buffer, size_t msg_size) {
  memcpy(&msg->pw_size, buffer, sizeof(int));

  if (msg->pw_size != msg_size - sizeof(int)) {
    return PARSE_ERROR;
  }
  int offset = sizeof(int);

  if (msg->pw_size > PWDEFAULTSIZE) {
    return PARSE_ERROR;
  }

  memcpy(msg->pw, buffer + offset, msg->pw_size);
  return msg_size;
}

static int parse_send_session(struct SendSessionMsg* msg, const char* buffer, size_t msg_size) {
  memcpy(&msg->session_id, buffer, sizeof(int));
  return msg_size;
}

static int parse_connect_to_session(struct ConnectToSessionMsg* msg, const char* buffer, size_t msg_size) {
  memcpy(&msg->session_id, buffer, sizeof(int));
  int offset = sizeof(int);

  memcpy(&msg->pw_size, buffer+offset, sizeof(int));
  offset += sizeof(int);

  if (msg->pw_size > PWDEFAULTSIZE) {
    return PARSE_ERROR;
  }

  //                                 pwsize      sessionid
  if (msg->pw_size != msg_size - sizeof(int) - sizeof(int)) {
    return PARSE_ERROR;
  }

  memcpy(msg->pw, buffer + offset, msg->pw_size);
  return msg_size;
}

static int parse_send_status(struct SendStatusMsg* msg, const char* buffer, size_t msg_size) {
  memcpy(&msg->status_code, buffer, sizeof(int));

  if (msg->status_code < 0 || msg->status_code >= CLIENT_STATUS_MAX) {
    return PARSE_ERROR;
  }

  return msg_size;
}

static int parse_notify_user(struct NotifyUserMsg* msg, const char* buffer, size_t msg_size) {
  memcpy(msg->ipv4, buffer, sizeof(msg->ipv4));
  int offset = sizeof(msg->ipv4);
  int isTerminated = 0;
  for (int i = 0; i < sizeof(msg->ipv4); i++) {
    if (msg->ipv4[i] == 0) {
      isTerminated = 1;
      break;
    }
  }

  if (!isTerminated) {
    return PARSE_ERROR;
  }

  memcpy(&msg->status_code, buffer + offset, sizeof(int));

  if (msg->status_code < 0 || msg->status_code >= CLIENT_STATUS_MAX) {
    return PARSE_ERROR;
  }

  return msg_size;
}

int parse_client_message(struct ClientMsg* msg, const char* buffer, size_t size) {
  // message can't contains msg size
  if (size < sizeof(int)) {
    return NOT_ENOUGH_DATA;
  }

  unsigned msg_size = 0;

  memcpy(&msg_size, buffer, sizeof(unsigned));

  if (msg_size > 1024 * 1024) {
    LOG_WARN("msg size is bigger than 1 mb: %d", msg_size);
    return PARSE_ERROR;
  }

  if(size < msg_size) {
    return NOT_ENOUGH_DATA;
  }

  int offset = sizeof(unsigned);
  short msg_id = 0;

  memcpy(&msg_id, buffer+offset, sizeof(short));
  offset += sizeof(short);
  int n = 0;
  switch (msg_id) {
    case CREATE_GAME_SESSION: {
      n = parse_create_session(&msg->create_game_session, buffer + offset, msg_size - offset);
      break;
    }
    case CONNECT_TO_SESSION: {
      n = parse_connect_to_session(&msg->connect_to_session, buffer + offset,  msg_size - offset);
      break;
    }
    default: {
      n = PARSE_ERROR;
      break;
    }
  }

  if (n <= 0) {
    return n;
  }

  return msg_size;
}

int parse_server_message(struct ServerMsg* msg, const char* buffer, size_t size) {
  // message can't contains msg size
  if (size < sizeof(int)) {
    return NOT_ENOUGH_DATA;
  }

  unsigned msg_size = 0;

  memcpy(&msg_size, buffer, sizeof(unsigned));

  if (msg_size > 1024 * 1024) {
    LOG_WARN("msg size is bigger than 1 mb: %d", msg_size);
    return PARSE_ERROR;
  }

  if(size < msg_size) {
    return NOT_ENOUGH_DATA;
  }

  int offset = sizeof(unsigned);
  short msg_id = 0;

  memcpy(&msg_id, buffer+offset, sizeof(short));
  offset += sizeof(short);
  int n = 0;
  switch (msg_id) {
    case SEND_SESSION: {
      n = parse_send_session(&msg->send_session, buffer + offset, msg_size - offset);
      break;
    }
    case SEND_STATUS: {
      n = parse_send_status(&msg->send_status, buffer + offset,  msg_size - offset);
      break;
    }
    case NOTIFY_USER: {
      n = parse_notify_user(&msg->notify_user, buffer + offset,  msg_size - offset);
      break;
    }
    default: {
      n = PARSE_ERROR;
      break;
    }
  }

  if (n <= 0) {
    return n;
  }

  return msg_size;
}


static int fill_create_session(const struct CreateGameSessionMsg* msg, char* buffer, size_t size) {
  int msg_size = msg->pw_size + sizeof(msg->pw_size);

  if (size < msg_size) {
    return NOT_ENOUGH_DATA;
  }

  int offset = 0;

  memcpy(buffer, &msg->pw_size, sizeof(msg->pw_size));
  offset += sizeof(msg->pw_size);

  memcpy(buffer + offset, msg->pw, msg->pw_size);

  return msg_size;
}

static int fill_send_session(const struct SendSessionMsg* msg, char* buffer, size_t size) {
  int msg_size = sizeof(msg->session_id);

  if (size < msg_size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, &msg->session_id, sizeof(msg->session_id));

  return msg_size;
}

static int fill_connect_to_session(const struct ConnectToSessionMsg* msg, char* buffer, size_t size) {
  int msg_size = sizeof(msg->pw_size) + sizeof(msg->session_id) + msg->pw_size;

  if (size < msg_size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, &msg->session_id, sizeof(msg->session_id));
  int offset = sizeof(msg->session_id);

  memcpy(buffer + offset, &msg->pw_size, sizeof(msg->pw_size));
  offset += sizeof(msg->pw_size);

  memcpy(buffer + offset, msg->pw, msg->pw_size);

  return msg_size;
}

static int fill_send_status(const struct SendStatusMsg* msg, char* buffer, size_t size) {
  int msg_size = sizeof(msg->status_code);

  if (size < msg_size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, &msg->status_code, sizeof(msg->status_code));

  return msg_size;
}

static int fill_notify_user(const struct NotifyUserMsg* msg, char* buffer, size_t size) {
  int msg_size = sizeof(msg->ipv4) + sizeof(msg->status_code);

  if (size < msg_size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, &msg->ipv4, sizeof(msg->ipv4));
  int offset = sizeof(msg->ipv4);

  memcpy(buffer + offset, &msg->status_code, sizeof(msg->status_code));

  return msg_size;
}

int fill_server_message(const struct ServerMsg* msg, char* buffer, size_t size) {
  int msg_size = 0;
  int min_size = sizeof(int) + sizeof(short);

  if (size < min_size) {
    return NOT_ENOUGH_DATA;
  }

  switch (msg->id) {
    case SEND_SESSION: {
      msg_size = fill_send_session(&msg->send_session, buffer + min_size, size - min_size);
      break;
    }
    case SEND_STATUS: {
      msg_size = fill_send_status(&msg->send_status, buffer + min_size, size - min_size);
      break;
    }
    case NOTIFY_USER: {
      msg_size = fill_notify_user(&msg->notify_user, buffer + min_size, size - min_size);
      break;
    }
    default: {
      LOG_ERROR("Invalid message id is set: %d", msg->id);
      abort();
    }
  }

  if (msg_size <= 0) {
    return msg_size;
  }

  msg_size += min_size;

  memcpy(buffer, &msg_size, sizeof(msg_size));
  int offset = sizeof(msg_size);

  memcpy(buffer + offset, &msg->id, sizeof(msg->id));

  return msg_size;
}

int fill_client_message(const struct ClientMsg* msg, char* buffer, size_t size) {
  int msg_size = 0;
  int min_size = sizeof(int) + sizeof(short);

  if (size < min_size) {
    return NOT_ENOUGH_DATA;
  }

  switch (msg->id) {
    case CREATE_GAME_SESSION: {
      msg_size = fill_create_session(&msg->create_game_session, buffer + min_size, size - min_size);
      break;
    }
    case CONNECT_TO_SESSION: {
      msg_size = fill_connect_to_session(&msg->connect_to_session, buffer + min_size, size - min_size);
      break;
    }
    default: {
      LOG_ERROR("Invalid message id is set: %d", msg->id);
      abort();
    }
  }

  if (msg_size <= 0) {
    return msg_size;
  }

  msg_size += min_size;

  memcpy(buffer, &msg_size, sizeof(msg_size));
  int offset = sizeof(msg_size);

  memcpy(buffer + offset, &msg->id, sizeof(msg->id));

  return msg_size;
}
