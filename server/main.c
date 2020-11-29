#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "bool.h"
#include "log.h"
#include "server.h"


static Server* s;

static void sigint(int signal) {
  (void)signal;
  server_stop(s);
}

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
  if (server_init(&server, host, port) < 0) {
    return EXIT_FAILURE;
  }

  s = &server;
  struct sigaction handler = {
    .sa_handler = sigint,
    .sa_mask = 0,
    .sa_flags = SA_RESTART
  };
  if (sigaction(SIGINT, &handler, NULL) == -1) {
    LOG_ERROR("Failed to install signal handler: %s", strerror(errno));
  }

  bool success = server_run(&server) == 0;
  server_close(&server);

  LOG_INFO("Closed %s", success ? "successfully" : "due to error");
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
