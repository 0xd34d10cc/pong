#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "connection.h"
#include "messages.h"

#define BUFFSIZE 512


void run(char* ip, char* port, struct ConnectionMap* con_map) {
  static int session_counter = 0;
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(port));
  addr.sin_addr.s_addr = inet_addr(ip);
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sock == -1) {
    perror("sock is not ready to be used");
    close(sock);
    return;
  }

  printf("start binding\n");

  if (bind(sock, (struct sockaddr*) &addr, sizeof addr) == -1) {
    perror("can't bind to the address");
    close(sock);
    return;
  }

  printf("start listening\n");
  if (listen(sock, 20) == -1) {
    perror("error is occured while initialization of listen()");
    return;
  }

  socklen_t socklen = sizeof addr;
  struct sockaddr_in player1_addr = {0};
  struct sockaddr_in player2_addr = {0};
  printf("starting accepting\n");
  int player1_sock = accept(sock, (struct sockaddr*)&player1_addr, &socklen);
  printf("Accept player1\n");
  int player2_sock = accept(sock, (struct sockaddr*)&player2_addr, &socklen);
  printf("Accept player2\n");

  if (player1_sock == -1 || player2_sock == -1) {
    perror("accept error");
    close(sock);
    return;
  }

  struct ConnectionStorage con_storage = {0};

  char* player1_str = inet_ntoa(player1_addr.sin_addr);
  memcpy(con_storage.player1_addr, player1_str, strlen(player1_str) + 1);

  char* player2_str = inet_ntoa(player2_addr.sin_addr);
  memcpy(con_storage.player2_addr, player2_str, strlen(player2_str) + 1);

  char buf[BUFFSIZE];
  ssize_t readden = recv(player1_sock, buf, BUFFSIZE, 0);

  if (readden == 0) {
    printf("session was killed by user (P1)\n");
    return;
  } else if (readden < 0) {
    perror("internal recv error (?)");
    return;
  } else {

    // parse messages
    // TODO: recv_all();

    int msg_size = 0;
    memcpy(&msg_size, buf, sizeof(int));

    char* msg = malloc(msg_size);
    memcpy(msg, buf, msg_size);

    enum MessageType msg_type = getMessageType(msg);

    if (msg_type == CreateGameSession) {
      struct CreateGameSessionMsg cgs_msg = {0};
      memcpy(&cgs_msg.pw_size,  msg + 4, sizeof(int));
      if (cgs_msg.pw_size > PWDEFAULTSIZE) {
        printf("password is shrinked to %d characters", PWDEFAULTSIZE);
        cgs_msg.pw_size = PWDEFAULTSIZE;
      }

      memcpy(&cgs_msg.pw, msg + 4 + sizeof(int), cgs_msg.pw_size);

      con_storage.pw_size = cgs_msg.pw_size;
      memcpy(con_storage.pw, cgs_msg.pw, cgs_msg.pw_size);
      int session_id = session_counter++;

      insert(con_map, session_id, &con_storage);
      //send_session(session_id);
    }
  }
}
