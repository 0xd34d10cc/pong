#include "tcp_stream.h"

#include <errno.h>
#include <string.h>
#include <stdbool.h>

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int tcp_init(TcpStream* stream, Reactor* loop) {
  int s = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
  if (s == -1) {
    return -1;
  }

  return tcp_from_socket(stream, loop, s);
}
#else
int tcp_init(TcpStream* stream, Reactor* loop) {
  // Initialize Winsock
  WSADATA wsa_data = { 0 };
  int result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (result != 0) {
    return -1;
  }

  int family = AF_INET;
  int type = SOCK_STREAM;
  int protocol = IPPROTO_TCP;
 
  SOCKET sock = socket(family, type, protocol);;
  // 0 is blocking, otherwise nonblocking
  u_long mode = 0;
  ioctlsocket(sock, FIONBIO, &mode);
  if (sock == INVALID_SOCKET) {
    return -1;
  }

  return tcp_from_socket(stream, loop, sock);
}
#endif


#ifndef WIN32
int tcp_from_socket(TcpStream* stream, Reactor* reactor, int socket) {
#else
int tcp_from_socket(TcpStream * stream, Reactor * reactor, SOCKET socket) {
#endif
  stream->state.fd = socket;
  stream->state.events = 0;
  stream->reactor = reactor;
  stream->received = 0;
  stream->to_send = 0;
  return reactor_register(reactor, &stream->state, 0);
}

void tcp_close(TcpStream* stream) {
  reactor_deregister(stream->reactor, &stream->state);
#ifndef WIN32
  close(stream->state.fd);
#else
  closesocket(stream->state.fd);
#endif
}

void tcp_shutdown(TcpStream* stream) {
  reactor_deregister(stream->reactor, &stream->state);
#ifndef WIN32
  shutdown(stream->state.fd, SHUT_RDWR);
#else
  shutdown(stream->state.fd, SD_BOTH);
#endif
}

int tcp_start_connect(TcpStream* stream, const char* ip, unsigned short port) {
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = inet_addr(ip);

  if (connect(stream->state.fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
    if (errno != EINPROGRESS) {
      return -1;
    }
  }
  return reactor_update(stream->reactor, &stream->state, IO_EVENT_WRITE);
}

int tcp_connect(TcpStream* stream) {
  int error = 0;
  socklen_t error_len = sizeof(error);
  if (getsockopt(stream->state.fd, SOL_SOCKET, SO_ERROR, &error, &error_len) == -1) {
    return -1;
  }
  reactor_update(stream->reactor, &stream->state, 0);
  return error;
}

int tcp_start_send(TcpStream* stream, const char* data, int size) {
  if (stream->to_send + size > sizeof(stream->output)) {
    return 0;
  }

  memcpy(stream->output + stream->to_send, data, size);
  if (reactor_update(stream->reactor, &stream->state, stream->state.events | IO_EVENT_WRITE) == -1) {
    return -1;
  }

  stream->to_send += size;
  return size;
}

int tcp_send(TcpStream* stream) {
  bool success = true;
  int total = 0;
  while (total != stream->to_send) {
#ifndef WIN32
    int n = send(stream->state.fd, stream->output + total, stream->to_send - total, MSG_NOSIGNAL);
#else
    int n = send(stream->state.fd, stream->output + total, stream->to_send - total, 0);
#endif
    if (n == -1) {
      success = errno == EWOULDBLOCK;
      break;
    }

    total += n;
  }

  memmove(stream->output, stream->output + total, stream->to_send - total);
  stream->to_send -= total;
  if (stream->to_send == 0) {
    if (reactor_update(stream->reactor, &stream->state, stream->state.events & ~IO_EVENT_WRITE) == -1) {
      return -1;
    }
  }

  return success ? 0 : -1;
}

int tcp_start_recv(TcpStream* stream) {
  return reactor_update(stream->reactor, &stream->state, stream->state.events | IO_EVENT_READ);
}

int tcp_recv(TcpStream* stream) {
  while (stream->received != sizeof(stream->input)) {
    int n = recv(stream->state.fd, stream->input + stream->received, sizeof(stream->input) - stream->received, 0);
    if (n == 0) {
      // Connection closed
      return 0;
    }

    if (n == -1) {
      if (errno == EWOULDBLOCK) {
        break;
      }
      return -1;
    }

    stream->received += n;
  }

  return 1;
}

int tcp_consume(TcpStream* stream, int n) {
  if (n > stream->received) {
    return -1;
  }

  memmove(stream->input, stream->input + n, stream->received - n);
  stream->received -= n;
  return 0;
}
