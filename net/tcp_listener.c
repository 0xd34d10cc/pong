#include "tcp_listener.h"

#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "tcp_stream.h"
#include "log.h"


int tcp_listener_init(TcpListener* listener, Reactor* reactor, const char* ip, unsigned short port) {
  int s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
  if (s == -1) {
    return -1;
  }

  int flag = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void*)&flag, sizeof(int)) == -1) {
    close(s);
    return -1;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = inet_addr(ip);
  if (bind(s, (struct sockaddr*)&address, sizeof(address)) == -1) {
    close(s);
    return -1;
  }

  if (listen(s, 16) == -1) {
    close(s);
    return -1;
  }

  listener->state.fd = s;
  listener->state.events = 0;
  listener->reactor = reactor;
  return reactor_register(listener->reactor, &listener->state, 0);
}

void tcp_listener_close(TcpListener* listener) {
  reactor_deregister(listener->reactor, &listener->state);
  close(listener->state.fd);
}

int tcp_listener_start_accept(TcpListener* listener) {
  return reactor_update(listener->reactor, &listener->state, IO_EVENT_READ);
}

int tcp_listener_accept(TcpListener* listener, TcpStream* accepted, struct sockaddr_in* address) {
  socklen_t address_len = sizeof(struct sockaddr_in);
  int socket = accept4(listener->state.fd, (struct sockaddr*)address, &address_len, SOCK_NONBLOCK);
  if (socket == -1) {
    if (errno == EWOULDBLOCK) {
      return 0;
    }
    return -1;
  }

  if (tcp_from_socket(accepted, listener->reactor, socket) == -1) {
    return -1;
  }

  return 1;
}
