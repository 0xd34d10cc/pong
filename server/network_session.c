#include "network_session.h"

#include <string.h>

#include <unistd.h>

void network_session_init(NetworkSession* session, int socket, struct sockaddr_in* address) {
  session->socket = socket;
  session->received = 0;
  session->sent = 0;
  memcpy(&session->address, address, sizeof(session->address));
  session->game = NULL;
}

void network_session_close(NetworkSession* session) {
  close(session->socket);
}
