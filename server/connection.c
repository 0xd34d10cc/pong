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
#include "log.h"


#define BUFFSIZE 512

int static session_counter = 0;

void get_ip_str(int sock, char* str) {
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  int res = getpeername(sock, (struct sockaddr *)&addr, &addr_size);
  strcpy(str, inet_ntoa(addr.sin_addr));
}

int accept_connection(int server_socket) {
  int addr_size = sizeof(struct sockaddr_in);
  int client_socket;
  struct sockaddr_in client_addr;
  
  int accepted_sock = (accept(server_socket, (struct sockaddr*) &client_addr, &addr_size));
  if (accepted_sock == -1) {
    perror("Accept error");
    return -1;
  }
  printf("accepted\n");
//  char addr_str[INET_ADDRSTRLEN];
//  inet_ntop(AF_INET, &client_addr, addr_str, INET_ADDRSTRLEN);
//  printf("accepted connection: %s /n", addr_str);
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

void send_status(enum ClientStatus status, int client_sock) {
  struct SendStatusMsg msg = {0};
  msg.id = SENDSTATUSID;
  msg.status_code = status;

  char buf[BUFFSIZE];

  //TODO serialize(char* buf, SendStatusMsg)
  int msg_size = 0;
  msg_size += sizeof(msg.id);
  msg_size += sizeof(msg.status_code);

  // 4 bytes for msg size
  msg_size += sizeof(int);


  // serialization
  int offset = 0;
  
  memcpy(buf + offset, &msg_size, sizeof(msg_size));
  offset += sizeof(int); 

  memcpy(buf + offset, &msg.id, sizeof(msg.id));
  offset += sizeof(msg.id);

  memcpy(buf + offset, &msg.status_code, sizeof(msg.status_code));

  int send_res = send(client_sock, buf, msg_size, 0);
  if (send_res == -1) {
    perror("send failed");
  }

  LOG_INFO("SendStatus message sent to user with status code: %d", msg.status_code);
}

void notify_user(enum ClientStatus status, int client_sock, char* addr_str) {
  struct NotifyUserMsg msg = {0};
  msg.status_code = status;
  memcpy(msg.ipv4, addr_str, INET_ADDRSTRLEN);
  msg.id = NOTIFYUSERID;
  
  char buf[BUFFSIZE];

  //TODO: Serialize(char* buf, NotifyUserMsg)
  int msg_size = 0;

  msg_size += sizeof(msg.id);
  msg_size += sizeof(msg.status_code);
  msg_size += sizeof(msg.ipv4); // INET_ADDRSTRLEN
  
  // 4 bytes for msg size
  msg_size += sizeof(int);


  // serialization
  int offset = 0;
  memcpy(buf + offset, &msg_size, sizeof(msg_size));
  offset += sizeof(msg_size);

  memcpy(buf + offset, &msg.id, sizeof(msg.id));
  offset += sizeof(msg.id);

  memcpy(buf + offset, &msg.ipv4, sizeof(msg.ipv4));
  offset += sizeof(msg.ipv4);

  memcpy(buf + offset, &msg.status_code, sizeof(msg.status_code));

  // send to client

  int send_res = send(client_sock, buf, msg_size, 0);
  if (send_res == -1) {
    perror("send failed");
  }

  LOG_INFO("NotifyUser message sent to user with status code: %d", msg.status_code);
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

  LOG_INFO("SendSession message sent to user with id: %d", session_id);
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
    
    if (readden < sizeof(int)) {
      LOG_WARN("Invalid message received. readden: %d is less than sizeof int", readden);
      return;
    }
    memcpy(&msg_size, buf, sizeof(int));

    if (readden < msg_size) {
      LOG_WARN("Invalid message received. readden: %d is less then msg_size: %d", readden, msg_size);
      return;
    }
    char* msg = malloc(msg_size);
    memcpy(msg, buf, msg_size);

    enum MessageType msg_type = getMessageType(msg);

    if (CreateGameSession == msg_type) {
      LOG_INFO("CreateGameSession received");
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
        LOG_INFO("password is shrinked to %d characters", PWDEFAULTSIZE);
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
      LOG_INFO("ConnectToSession message received");

      int offset = 4;
      struct ConnectToSessionMsg cts_msg = {0};
      memcpy(&cts_msg.id, msg + offset, sizeof(cts_msg.id));
      offset += sizeof(cts_msg.id);

      memcpy(&cts_msg.session_id, msg + offset, sizeof(cts_msg.session_id));
      offset += sizeof(cts_msg.session_id);

      memcpy(&cts_msg.pw_size, msg + offset, sizeof(cts_msg.pw_size));
      offset += sizeof(cts_msg.pw_size);

      if (cts_msg.pw_size > PWDEFAULTSIZE) {
        LOG_INFO("password is shrinked to %d characters", PWDEFAULTSIZE);
        cts_msg.pw_size = PWDEFAULTSIZE;
      }
      memcpy(&cts_msg.pw, msg + offset, cts_msg.pw_size);

      if (cts_msg.session_id >= session_counter) {
        LOG_WARN("received session id: %d is bigger than current session id: %d, ignoring", cts_msg.session_id, session_counter);
        send_status(WrongSessionId, client_sock);
        return;
      }

      struct ConnectionStorage* storage = get_storage(map, cts_msg.session_id);
      if (Created != storage->status) {
        LOG_WARN("client is trying to connect to already closed or running session");
        send_status(WrongSessionId, client_sock);
        return;
      }

      if (storage->player1_sock == client_sock) {
        LOG_WARN("Same sockets for 2 players.");
        send_status(WrongSessionId, client_sock);
        return;
      }
      // check password
      if (storage->pw_size != cts_msg.pw_size) {
        LOG_WARN("invalid password size. Expected: %d, received: %d", storage->pw_size, cts_msg.pw_size);
        send_status(WrongPassword, client_sock);
        return;
      }
      
      if (0 > storage->pw_size) {
        int res = memcmp(storage->pw, cts_msg.pw_size, storage->pw_size);
        if (0 != res) {
          LOG_WARN("Wrong passwords.");
          send_status(WrongPassword, client_sock);
          return;
        }
      }

      storage->player2_sock = client_sock;
      storage->status = Pending;
      
      send_status(Connected, client_sock);

      char addr[INET_ADDRSTRLEN];
      get_ip_str(client_sock, addr);
      notify_user(Connected, storage->player1_sock, addr);
    }
  }
}

void run(char* ip, char* port, struct ConnectionMap* con_map) {
  static int session_counter = 0;
  int server_sock = setup_server(ip, port);
  if (-1 == server_sock) return;

  fd_set current_sockets;
  fd_set ready_sockets;

  FD_ZERO(&current_sockets);
  FD_SET(server_sock, &current_sockets);

  while (1) {
    memcpy(&ready_sockets, &current_sockets, sizeof(current_sockets)); 
    //ready_sockets = current_sockets;
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
          
          FD_SET(client_sock, &current_sockets);
        } 

        // handle connection
        else {
          handle_connection(i, con_map);
          FD_CLR(i, &current_sockets);
        }
      }
    }
  }
}


