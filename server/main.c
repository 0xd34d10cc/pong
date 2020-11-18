#include <stdlib.h>

#include "log.h"
#include "server.h"


int main(int argc, char* argv[]) {
  // ./server 127.0.0.1 1337
  if (argc > 3) {
    LOG_ERROR("Too many arguments, expected at most 2");
    return EXIT_FAILURE;
  }

  const char* host = argc < 2 ? "127.0.0.1" : argv[1];
  unsigned short port = 1337;
  if (argc == 3) {
    port = (unsigned short)atoi(argv[2]);

    if (port == 0) {
      LOG_ERROR("%s is not a valid port number", argv[2]);
      return EXIT_FAILURE;
    }
  }

  LOG_INFO("Starting at %s:%d", host, port);
  Server server;
  server_init(&server, host, port);
  server_run(&server);
  server_close(&server);

  return EXIT_SUCCESS;
}
