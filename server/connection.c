#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "connection.h"
#include "messages.h"

#define BUFFSIZE 512

int static session_counter = 0;

int accept_connection(int server_socket) {
  int addr_size = sizeof(struct sockaddr_in);
  int client_socket;
  struct sockaddr_in client_addr;
  
  int accepted_sock = (accept(server_socket, (struct sockaddr*) &client_addr, &addr_size));
  if (accepted_sock == -1) {
    perror("Accept error");
    return -1;
  }

  return accepted_sock;
}

int setup_server(char* ip, char* port) {
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(port));
  addr.sin_addr.s_addr = inet_addr(ip);
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sock == -1) {
    perror("sock is not ready to be used");
    close(sock);
    return -1;
  }

  printf("start binding\n");

  if (bind(sock, (struct sockaddr*) &addr, sizeof addr) == -1) {
    perror("can't bind to the address");
    close(sock);
    return -1;
  }

  printf("start listening\n");
  if (listen(sock, 20) == -1) {
    perror("error is occured while initialization of listen()");
    return -1;
  }
  return sock;
}

void send_session(int session_id, int client_sock) {
  struct SendSessionMsg msg = {0};
  msg.id = SENDSESSIONID; 
  msg.session_id = session_id;

  char buf[BUFFSIZE];

  //TODO: serialize(char* buf, SendSessionMsg)
  int msg_size = 0;

  msg_size += sizeof(msg.id);
  msg_size += sizeof(msg.session_id);
  
  // 4 bytes for msg size
  msg_size += sizeof(int);
  

  // serialization
  int offset = 0;
  memcpy(buf + offset, &msg_size, sizeof(int));
  offset += sizeof(int);
  memcpy(buf + offset, &msg.id, sizeof(short));
  offset += sizeof(short);
  memcpy(buf+ offset, &msg.session_id, sizeof(int));
  
  // send to client
  
  int send_res = send(client_sock, buf, msg_size, 0);
  if (send_res == -1) {
    perror("send failed");
  }
}


void handle_connection(int client_sock, struct ConnectionMap* map) {
  char buf[BUFFSIZE];
  ssize_t readden = recv(client_sock, buf, BUFFSIZE, 0);

  if (readden == 0) {
    printf("session was killed by user (P1)\n");
    return;
  } else if (readden < 0) {
    perror("internal recv error (?)");
    return;
  } else {

    int msg_size = 0;
    memcpy(&msg_size, buf, sizeof(int));

    char* msg = malloc(msg_size);
    memcpy(msg, buf, msg_size);

    enum MessageType msg_type = getMessageType(msg);

    if (CreateGameSession == msg_type) {
      // new connection, so we need to create new ConnectionStorage for it
      struct ConnectionStorage con_storage = {0}; 
      con_storage.status = Created;
      struct CreateGameSessionMsg cgs_msg = {0};
      // each message contains 4 bytes of it's size
      int offset = 4;

      con_storage.player1_sock = client_sock;
      memcpy(&cgs_msg.pw_size, msg + offset, sizeof(int));
      
      offset += sizeof(int);

      if (cgs_msg.pw_size > PWDEFAULTSIZE) {
        printf("password is shrinked to %d characters", PWDEFAULTSIZE);
        cgs_msg.pw_size = PWDEFAULTSIZE;
      }
      
      memcpy(&cgs_msg.pw, msg + offset, cgs_msg.pw_size);

      con_storage.pw_size = cgs_msg.pw_size;
      memcpy(con_storage.pw, cgs_msg.pw, cgs_msg.pw_size);
      int session_id = session_counter++;

      insert(map, session_id, &con_storage);
      send_session(session_id, client_sock);
    }

    if (ConnectToSession == msg_type) {
      // TODO: implement
    }
  }
}

void run(char* ip, char* port, struct ConnectionMap* con_map) {
  static int session_counter = 0;
  int server_sock = setup_server(ip, port);
  if (server_sock == -1) return;

  fd_set current_sockets;
  fd_set ready_sockets;

  FD_ZERO(&current_sockets);
  FD_SET(server_sock, &current_sockets);

  while (1) {
  
    ready_sockets = current_sockets;
    // just read for now
    if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
      perror("select error");
      return;
    }

    for (int i = 0; i < FD_SETSIZE; i++) {
      if (FD_ISSET(i, &ready_sockets)) {
        
        // new connection
        if(i == server_sock) {
          
          int client_sock = accept_connection(i);
          if (client_sock == -1) return;

          FD_CLR(client_sock, &current_sockets);
        } 

        // handle connection
        else {
          handle_connection(i, &con_map);
          FD_CLR(i, &current_sockets);
        }
      }
    }
  }
}


