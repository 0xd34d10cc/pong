#include "network_session.h"

void network_session_init(NetworkSession* session, int socket, struct sockaddr_in* addr) {
  session->socket = socket;
  session->received = 0;
  session->sent = 0;
  memcpy(&session->sock_addr, addr, sizeof(sockaddr_in));
  session->game = NULL;
}
