#include "network_session.h"

#include <unistd.h>

void network_session_init(NetworkSession* session, int socket, struct sockaddr_in* addr) {
  session->socket = socket;
  session->received = 0;
  session->sent = 0;
  memcpy(&session->sock_addr, addr, sizeof(struct sockaddr_in));
  session->game = NULL;
}

void network_session_close(NetworkSession* session) {
  close(session->socket);
}
