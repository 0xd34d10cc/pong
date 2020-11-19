#include "network_session.h"

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static void set_nonblocking(int socket) {
  int flags = fcntl(socket, F_GETFL, 0);
  if (flags == -1) {
    LOG_ERROR("Can't get flags from socket");
    return;
  }

  flags |= O_NONBLOCK;
  if (fcntl(socket, F_SETFL, flags) != 0) {
    LOG_ERROR("Can't set flags for socket");
  }
}

int network_session_init(NetworkSession* session) {
  int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == -1) {
    LOG_ERROR("Failed to create socket: %s", strerror(errno));
    return -1;
  }

  set_nonblocking(s);
  session->socket = s;
  session->events = 0;
  session->received = 0;
  session->to_send = 0;
  return 0;
}

void network_session_close(NetworkSession* session) {
  close(session->socket);
}

int network_session_connect(NetworkSession* session, const char* host, unsigned short port) {
  struct sockaddr_in addr = { 0 };
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr(host);

  int res = connect(session->socket, (struct sockaddr*)&addr, sizeof(addr));

  if (res != 0 && res != EINPROGRESS) {
    LOG_WARN("connect failed with error: %s", strerror(errno));
    return -1;
  }

  return 0;
}