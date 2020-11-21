#include "messages.h"

#include <string.h>

#include "panic.h"
#include "bool.h"

#define NOT_ENOUGH_DATA 0
#define PARSE_ERROR -1


static int read_create_session(CreateSession* message, const char* buffer, size_t msg_size) {
  if (msg_size == 0 || msg_size > sizeof(message->password) || buffer[msg_size - 1] != '\0') {
    return PARSE_ERROR;
  }

  memcpy(message->password, buffer, msg_size);
  return msg_size;
}

static int read_session_created(SessionCreated* message, const char* buffer, size_t msg_size) {
  if (msg_size != sizeof(message->session_id)) {
    return PARSE_ERROR;
  }

  memcpy(&message->session_id, buffer, sizeof(message->session_id));
  return msg_size;
}

static int read_join_session(JoinSession* message, const char* buffer, size_t message_size) {
  if (message_size < sizeof(message->session_id)) {
    return PARSE_ERROR;
  }

  memcpy(&message->session_id, buffer, sizeof(message->session_id));

  size_t data_left = message_size - sizeof(message->session_id);
  if (data_left == 0 || data_left > sizeof(message->password) || buffer[data_left - 1] != '\0') {
    return PARSE_ERROR;
  }

  memcpy(message->password, buffer + sizeof(message->session_id), data_left);
  return message_size;
}

static int read_session_joined(SessionJoined* message, const char* buffer, size_t message_size) {
  if (message_size == 0 || message_size > sizeof(message->ipv4) || buffer[message_size - 1] != '\0') {
    return PARSE_ERROR;
  }

  memcpy(message->ipv4, buffer, message_size);
  return message_size;
}

static int read_error_status(ErrorStatus* message, const char* buffer, size_t message_size) {
  if (message_size != sizeof(message->status)) {
    return PARSE_ERROR;
  }

  memcpy(&message->status, buffer, sizeof(message->status));
  return message_size;
}

static int read_message(void* message, const char* buffer, size_t size, bool is_server) {
  unsigned message_size = 0;
  if (size < sizeof(message_size)) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(&message_size, buffer, sizeof(message_size));
  if (message_size > MAX_MESSAGE_SIZE) {
    return PARSE_ERROR;
  }

  if (message_size > size) {
    return NOT_ENOUGH_DATA;
  }

  int offset = sizeof(message_size);

  unsigned short message_id = 0;
  memcpy(&message_id, buffer + offset, sizeof(message_id));
  offset += sizeof(message_id);

  int n = -1;
  if (is_server) {
    ServerMessage* server_message = (ServerMessage*)message;
    switch (message_id) {
      case SESSION_CREATED:
        n = read_session_created(&server_message->session_created, buffer + offset, message_size - offset);
        break;
      case SESSION_JOINED:
        n = read_session_joined(&server_message->session_joined, buffer + offset, message_size - offset);
        break;
      case ERROR_STATUS:
        n = read_error_status(&server_message->error_status, buffer + offset, message_size - offset);
        break;
      default: {
        n = PARSE_ERROR;
        break;
      }
    }

    server_message->id = message_id;
  }
  else {
    ClientMessage* client_message = (ClientMessage*)message;
    switch (message_id) {
      case CREATE_SESSION:
        n = read_create_session(&client_message->create_session, buffer + offset, message_size - offset);
        break;
      case JOIN_SESSION:
        n = read_join_session(&client_message->join_session, buffer + offset,  message_size - offset);
        break;
      default:
        n = PARSE_ERROR;
        break;
    }

    client_message->id = message_id;
  }

  if (n <= 0) {
    return n;
  }

  return message_size;
}

int client_message_read(ClientMessage* message, const char* buffer, size_t size) {
  return read_message(message, buffer, size, false);
}

int server_message_read(ServerMessage* message, const char* buffer, size_t size) {
  return read_message(message, buffer, size, true);
}


static int write_create_session(const CreateSession* message, char* buffer, size_t size) {
  int message_size = strlen(message->password) + 1;
  if (message_size > size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, message->password, message_size);
  return message_size;
}

static int write_session_created(const SessionCreated* message, char* buffer, size_t size) {
  int message_size = sizeof(message->session_id);
  if (message_size > size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, &message->session_id, sizeof(message->session_id));
  return message_size;
}

static int write_join_session(const JoinSession* message, char* buffer, size_t size) {
  size_t password_len = strlen(message->password) + 1;
  int message_size = sizeof(message->session_id) + password_len;
  if (message_size > size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, &message->session_id, sizeof(message->session_id));
  memcpy(buffer + sizeof(message->session_id), message->password, password_len + 1);
  return message_size;
}

static int write_session_joined(const SessionJoined* message, char* buffer, size_t size) {
  int message_size = strlen(message->ipv4) + 1;
  if (message_size > size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, message->ipv4, message_size);
  return message_size;
}

static int write_error_status(const ErrorStatus* message, char* buffer, size_t size) {
  int message_size = sizeof(message->status);
  if (message_size > size) {
    return NOT_ENOUGH_DATA;
  }

  memcpy(buffer, &message->status, sizeof(message->status));
  return message_size;
}

static int write_message(const void* message, char* buffer, size_t size, bool is_server) {
  // message size + message id
  int min_size = sizeof(unsigned) + sizeof(unsigned short);
  if (size < min_size) {
    return NOT_ENOUGH_DATA;
  }

  int message_size = 0;
  unsigned short message_id = -1;
  if (is_server) {
    ServerMessage* server_message = (ServerMessage*)message;
    switch (server_message->id) {
      case SESSION_CREATED:
        message_size = write_session_created(&server_message->session_created, buffer + min_size, size - min_size);
        break;
      case SESSION_JOINED:
        message_size = write_session_joined(&server_message->session_joined, buffer + min_size, size - min_size);
        break;
      case ERROR_STATUS:
        message_size = write_error_status(&server_message->error_status, buffer + min_size, size - min_size);
        break;
      default:
        PANIC("Unhandled message id: %d", server_message->id);
        break;
    }
    message_id = server_message->id;
  }
  else {
    ClientMessage* client_message = (ClientMessage*)message;
    switch (client_message->id) {
      case CREATE_SESSION:
        message_size = write_create_session(&client_message->create_session, buffer + min_size, size - min_size);
        break;
      case JOIN_SESSION:
        message_size = write_join_session(&client_message->join_session, buffer + min_size, size - min_size);
        break;
      default:
        PANIC("Unhandled message id: %d", client_message->id);
        break;
    }
    message_id = client_message->id;
  }

  if (message_size <= 0) {
    return message_size;
  }

  message_size += min_size;

  memcpy(buffer, &message_size, sizeof(message_size));
  memcpy(buffer + sizeof(message_size), &message_id, sizeof(message_id));
  return message_size;
}

int client_message_write(const ClientMessage* message, char* buffer, size_t size) {
  return write_message(message, buffer, size, false);
}

int server_message_write(const ServerMessage* message, char* buffer, size_t size) {
  return write_message(message, buffer, size, true);
}
