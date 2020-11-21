#ifndef TCP_STREAM_H
#define TCP_STREAM_H

#include "reactor.h"

#define NET_BUFFER_SIZE 512


typedef struct TcpStream {
  // IO state: socket and list of subscribed events
  // WARNING: must be a first field
  Evented state;
  // A reactor to which this tcp stream is bound
  Reactor* reactor;

  // Buffer to received data
  char input[NET_BUFFER_SIZE];
  int received;

  // Buffer to sent data
  char output[NET_BUFFER_SIZE];
  int to_send;
} TcpStream;

// Create a new non-blocking tcp stream
int tcp_init(TcpStream* stream, Reactor* loop);

// Init tcp stream from existing socket (e.g. from accept())
int tcp_from_socket(TcpStream* stream, Reactor* loop, int socket);

// Close tcp stream
void tcp_close(TcpStream* stream);

// Start a connect operation
int tcp_start_connect(TcpStream* stream, const char* ip, unsigned short port);

// Process a connect operation
// Requires: IO_EVENT_WRITE
int tcp_connect(TcpStream* stream);

// Start send() operation
// Returns:
//  -1    on error
//   0    if there is not enough space in output buffer
//   size on success
int tcp_start_send(TcpStream* stream, const char* data, int size);

// Process send() operation
// Requires: IO_EVENT_WRITE
int tcp_send(TcpStream* stream);

// Start read() operation
int tcp_start_recv(TcpStream* stream);

// Process recv() operation
// Requres: IO_EVENT_READ
// Returns:
//  -1 on error
//  0  if stream have been closed
//  1  on success, check stream->received for number of bytes in the input buffer
int tcp_recv(TcpStream* stream);

// Consume |n| bytes from input buffer
int tcp_consume(TcpStream* stream, int n);

#endif // TCP_STREAM_H
