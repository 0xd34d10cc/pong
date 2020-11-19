#include "network_session.h"

#include <string.h>

#include <unistd.h>

void network_session_init(NetworkSession* session, int socket, struct sockaddr_in* address) {
  session->socket = socket;
  session->events = 0;
  session->received = 0;
  session->to_send = 0;
  memcpy(&session->address, address, sizeof(session->address));
  session->game = NULL;
}

void network_session_close(NetworkSession* session) {
  close(session->socket);
}
