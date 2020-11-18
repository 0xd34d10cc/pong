#include <stdio.h>
#include <stdlib.h>

#include "messages.h"
#include "connection_utils.h"
#include "connection.h"

#define SERVERPORT 1337

#define MAXMSGSIZE 120

int main(int argc, char* argv[]) {
  // ./server 127.0.0.1 1337
  if (argc != 3) {
    printf("invalid ip or port\n");
    return -1;
  }

  LOG_INFO("starting with %s : %s", argv[1], argv[2]);
  struct ConnectionMap* con_map = malloc(sizeof(struct ConnectionMap));
  run(argv[1], argv[2],  con_map);

  map_destroy(con_map);
  return 0;
}
