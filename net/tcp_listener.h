#ifndef TCP_LISTENER_H
#define TCP_LISTENER_H

#include "reactor.h"

typedef struct TcpStream TcpStream;
struct sockaddr_in;

typedef struct TcpListener {
  Evented state;
  Reactor* reactor;
} TcpListener;

// Initialize tcp listener
int tcp_listener_init(TcpListener* listener, Reactor* reactor, const char* ip, unsigned short port);

void tcp_listener_close(TcpListener* listener);

// Start accept() operation
int tcp_listener_start_accept(TcpListener* listener);
int tcp_listener_stop_accept(TcpListener* listener);

// Process accept() operation
// Requires: IO_EVENT_READ
// Returns:
//  -1 on error
//  0  if there are no more clients to accept
//  1  on success and |stream| is initialized with accepted client
int tcp_listener_accept(TcpListener* listener, TcpStream* stream, struct sockaddr_in* address);

#endif // TCP_LISTENER_H
