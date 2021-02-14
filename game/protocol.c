#include "protocol.h"

#include <string.h>
#include <stdbool.h>

#include "panic.h"


static const size_t HEADER_SIZE = sizeof(unsigned short) + sizeof(unsigned short);

#define READ(field)                               \
  do {                                            \
    if (offset + sizeof(field) > len) {           \
      return -1;                                  \
    }                                             \
    memcpy(&field, data + offset, sizeof(field)); \
    offset += sizeof(field);                      \
  } while (0)

#define READ_ENDING_STR(field)                                                      \
  do {                                                                              \
    if (offset == len || (len - offset) > sizeof(field) || data[len - 1] != '\0') { \
      return -1;                                                                    \
    }                                                                               \
    memcpy(field, data + offset, len - offset);                                     \
    offset = len;                                                                   \
  } while (0)

static int read_message(void* message, const char* data, size_t size, bool is_server) {
  if (size < HEADER_SIZE) {
    return 0;
  }

  unsigned short len;
  unsigned short id;
  memcpy(&len, data, sizeof(len));
  memcpy(&id, data + sizeof(len), sizeof(id));

  if (len > MAX_MESSAGE_SIZE) {
    // The message is too big
    return -1;
  }

  if (size < HEADER_SIZE + len) {
    // Not enough data in the buffer
    return 0;
  }

  int offset = 0;
  data += HEADER_SIZE;

  if (is_server) {
    ServerMessage* server_message = (ServerMessage*)message;
    switch (id) {
      case LOBBY_CREATED:
        READ(server_message->lobby_created.id);
        break;
      case LOBBY_JOINED:
        READ_ENDING_STR(server_message->lobby_joined.ipv4);
        break;
      case ERROR_STATUS:
        READ(server_message->error.status);
        break;
      case SERVER_UPDATE:
        READ(server_message->server_update.player_position);
        READ(server_message->server_update.opponent_position);
        READ(server_message->server_update.ball_position);
        break;
      case GAME_STATE_UPDATE:
        READ(server_message->game_state_update.state);
        break;
      default:
        return -1;
    }

    server_message->id = id;
  }
  else {
    ClientMessage* client_message = (ClientMessage*)message;
    switch (id) {
      case CREATE_LOBBY:
        READ_ENDING_STR(client_message->create_lobby.password);
        break;
      case JOIN_LOBBY:
        READ(client_message->join_lobby.id);
        READ_ENDING_STR(client_message->join_lobby.password);
        break;
      case CLIENT_UPDATE:
        READ(client_message->client_update.speed);
        break;

      case CLIENT_STATE_UPDATE:
        READ(client_message->client_state_update.state);
        break;
      default:
        return -1;
    }

    client_message->id = id;
  }

  return offset == len ? HEADER_SIZE + len : -1;
}

int client_message_read(ClientMessage* message, const char* data, size_t size) {
  return read_message(message, data, size, false);
}

int server_message_read(ServerMessage* message, const char* data, size_t size) {
  return read_message(message, data, size, true);
}


#define WRITE(field)                              \
  do {                                            \
    if (offset + sizeof(field) > size) {          \
      return 0;                                   \
    }                                             \
    memcpy(data + offset, &field, sizeof(field)); \
    offset += sizeof(field);                      \
  } while (0)                                     \

#define WRITE_STR(field)               \
  do {                                 \
    int len = strlen(field) + 1;       \
    if (offset + len > size) {         \
      return 0;                        \
    }                                  \
    memcpy(data + offset, field, len); \
    offset += len;                     \
  } while (0)


static int write_message(const void* message, char* data, size_t size, bool is_server) {
  if (size < HEADER_SIZE) {
    return 0;
  }

  char* header = data;

  int offset = 0;
  unsigned short id;

  data += HEADER_SIZE;
  if (is_server) {
    ServerMessage* server_message = (ServerMessage*)message;
    switch (server_message->id) {
      case LOBBY_CREATED:
        WRITE(server_message->lobby_created.id);
        break;
      case LOBBY_JOINED:
        WRITE_STR(server_message->lobby_joined.ipv4);
        break;
      case ERROR_STATUS:
        WRITE(server_message->error.status);
        break;
      case SERVER_UPDATE:
        WRITE(server_message->server_update.player_position);
        WRITE(server_message->server_update.opponent_position);
        WRITE(server_message->server_update.ball_position);
        break;
      case GAME_STATE_UPDATE:
        WRITE(server_message->game_state_update.state);
        break;
      default:
        PANIC("Unhandled message id: %d", server_message->id);
        break;
    }

    id = server_message->id;
  }
  else {
    ClientMessage* client_message = (ClientMessage*)message;
    switch (client_message->id) {
      case CREATE_LOBBY:
        WRITE_STR(client_message->create_lobby.password);
        break;
      case JOIN_LOBBY:
        WRITE(client_message->join_lobby.id);
        WRITE_STR(client_message->join_lobby.password);
        break;
      case CLIENT_UPDATE:
        WRITE(client_message->client_update.speed);
        break;

      case CLIENT_STATE_UPDATE:
        WRITE(client_message->client_state_update.state);
        break;
      default:
        PANIC("Unhandled message id: %d", client_message->id);
        break;
    }

    id = client_message->id;
  }

  unsigned short len = (unsigned short)offset;
  memcpy(header, &len, sizeof(len));
  memcpy(header + sizeof(len), &id, sizeof(id));
  return HEADER_SIZE + len;
}

int client_message_write(const ClientMessage* message, char* data, size_t size) {
  return write_message(message, data, size, false);
}

int server_message_write(const ServerMessage* message, char* data, size_t size) {
  return write_message(message, data, size, true);
}
