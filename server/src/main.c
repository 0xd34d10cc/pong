#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "messages.h"
#include "connection_utils.h"
#include "connection.h"

#define SERVERPORT 1337

#define MAXMSGSIZE 120

enum StatusCode {
  OK = 0,
  WrongSessionID,
  WrongPassword
};

void handleCreateGameSession(struct CreateGameSessionMsg* msg, struct ConnectionMap* con_map) {
  
}

int main(int argc, char* argv[]) {
  static int session_counter = 0;
  // ./server 127.0.0.1 1337
  printf("starting with %s : %s\n", argv[1], argv[2]);
  if (argc != 3) {
    printf("invalid ip or port\n");
    return -1;
  }
  
  struct ConnectionMap con_map = {0};

  run(argv[1], argv[2],  &con_map);

  return 0;
}
