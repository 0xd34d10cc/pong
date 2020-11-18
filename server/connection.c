#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "connection.h"
#include "messages.h"
#include "log.h"
#include "network_session.h"

// FIXME: we already have address from accept() call. There is no need for getpeername()
static void get_ip_str(int sock, char* str) {
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  int res = getpeername(sock, (struct sockaddr *)&addr, &addr_size);
  strcpy(str, inet_ntoa(addr.sin_addr));
}

static void set_nonblock_flag(int sock) {
  int flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    LOG_ERROR("Can't get flags from socket");
    return;
  }
  flags |= O_NONBLOCK;
  if (fcntl(sock, F_SETFL, flags) != 0) {
    LOG_ERROR("Can't set flags for socket");
  }
}

static int accept_connection(int server_socket) {
  socklen_t addr_size = sizeof(struct sockaddr_in);
  int client_socket;
  struct sockaddr_in client_addr;

  int accepted_sock = (accept(server_socket, (struct sockaddr*) &client_addr, &addr_size));
  if (accepted_sock == -1) {
    LOG_ERROR("Accept error: %s", strerror(errno));
    return -1;
  }

  set_nonblock_flag(accepted_sock);
  LOG_INFO("Connection accepted");

  return accepted_sock;
}

static int setup_server(char* ip, char* port) {
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(port));
  addr.sin_addr.s_addr = inet_addr(ip);
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock == -1) {
    LOG_ERROR("sock is not ready to be used: %s", strerror(errno));
    close(sock);
    return -1;
  }

  set_nonblock_flag(sock);
  LOG_INFO("start binding");

  if (bind(sock, (struct sockaddr*) &addr, sizeof addr) == -1) {
    LOG_ERROR("can't bind to the address: %s", strerror(errno));
    close(sock);
    return -1;
  }

  LOG_INFO("start listening");
  if (listen(sock, 20) == -1) {
    LOG_ERROR("Error is occured while initialization of listen(): %s", strerror(errno));
    close(sock);
    return -1;
  }
  return sock;
}

static void send_status(enum ClientStatus status, int client_sock) {
  struct SendStatusMsg msg = {0};
  msg.id = SEND_STATUS;
  msg.status_code = status;

  // FIXME: use same connection-local buffer for any IO
  char buf[BUFSIZE];

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

  // TODO: check for incomplete send (i.e. sand_res < msg_size)
  int send_res = send(client_sock, buf, msg_size, 0);
  if (send_res == -1) {
    LOG_ERROR("Send failed: %s", strerror(errno));
  }

  LOG_INFO("SendStatus message sent to user with status code: %d", msg.status_code);
}

static void notify_user(enum ClientStatus status, int client_sock, char* addr_str) {
  struct NotifyUserMsg msg = {0};
  msg.status_code = status;
  memcpy(msg.ipv4, addr_str, INET_ADDRSTRLEN);
  msg.id = NOTIFY_USER;

  char buf[BUFSIZE];

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
    LOG_ERROR("send failed: %s", strerror(errno));
  }

  LOG_INFO("NotifyUser message sent to user with status code: %d", msg.status_code);
}

static void send_session(int session_id, int client_sock) {
  struct SendSessionMsg msg = {0};
  msg.id = SEND_SESSION;
  msg.session_id = session_id;

  char buf[BUFSIZE];

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
    LOG_ERROR("send failed: %s", strerror(errno));
  }

  LOG_INFO("SendSession message sent to user with id: %d", session_id);
}

static void deserialize_cts(struct ConnectToSessionMsg* msg, char* buf) {
  int offset = 4;
  memcpy(&msg->id, buf + offset, sizeof(msg->id));
  offset += sizeof(msg->id);

  memcpy(&msg->session_id, buf + offset, sizeof(msg->session_id));
  offset += sizeof(msg->session_id);

  memcpy(&msg->pw_size, buf + offset, sizeof(msg->pw_size));
  offset += sizeof(msg->pw_size);

  if (msg->pw_size > PWDEFAULTSIZE) {
    LOG_INFO("password is shrinked to %d characters", PWDEFAULTSIZE);
    msg->pw_size = PWDEFAULTSIZE;
  }
  memcpy(&msg->pw, buf + offset, msg->pw_size);
}

static void deserialize_cgs(struct CreateGameSessionMsg* msg, char* buf) {
  int offset = 4;
  memcpy(&msg->pw_size, buf + offset, sizeof(int));

  offset += sizeof(int);

  if (msg->pw_size > PWDEFAULTSIZE) {
    LOG_INFO("password is shrinked to %d characters", PWDEFAULTSIZE);
    msg->pw_size = PWDEFAULTSIZE;
  }

  memcpy(msg->pw, buf + offset, msg->pw_size);
}

static void handle_cgs(struct CreateGameSessionMsg* msg, struct ConnectionMap* map,
                       int session_id, int client_sock) {
  struct ConnectionStorage con_storage = {0};
  con_storage.status = Created;

  con_storage.player1_sock = client_sock;
  con_storage.pw_size = msg->pw_size;
  memcpy(con_storage.pw, msg->pw, msg->pw_size);

  // TODO: use session pool instead of map
  insert(map, session_id, &con_storage);
  send_session(session_id, client_sock);
}

static void handle_cts(struct ConnectToSessionMsg* msg, struct ConnectionMap* map,
                       int max_session_id, int client_sock) {
  if (msg->session_id >= max_session_id) {
    LOG_WARN("received session id: %d is bigger than current session id: %d, ignoring",
                                   msg->session_id, max_session_id);
    send_status(WrongSessionId, client_sock);
    return;
  }

  struct ConnectionStorage* storage = get_storage(map, msg->session_id);
  if (storage == NULL || Created != storage->status) {
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
  if (storage->pw_size != msg->pw_size) {
    LOG_WARN("invalid password size. Expected: %d, received: %d", storage->pw_size, msg->pw_size);
    send_status(WrongPassword, client_sock);
    return;
  }

  if (0 > storage->pw_size) {
    int res = memcmp(storage->pw, msg->pw, storage->pw_size);
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

// TODO: this function is too large, refactor
static void handle_connection(int client_sock, struct ConnectionMap* map, int* session_counter) {
  char buf[BUFSIZE];
  ssize_t readden = recv(client_sock, buf, BUFSIZE, 0);

  if (readden == 0) {
    LOG_WARN("session was killed by user (P1)");
    return;
  } else if (readden < 0) {
    LOG_ERROR("internal recv error (?): %s", strerror(errno));
    return;
  } else {
    int msg_size = 0;

    // FIXME: this is valid case and we should not just throw off the buffer after that happens
    //        instead, we should just wait for client to send more data
    if (readden < sizeof(int)) {
      LOG_WARN("Invalid message received. readden: %d is less than sizeof int", readden);
      return;
    }
    memcpy(&msg_size, buf, sizeof(int));

    if (readden < msg_size) {
      LOG_WARN("Invalid message received. readden: %d is less then msg_size: %d", readden, msg_size);
      return;
    }

    enum MessageType msg_type = getMessageType(buf);

    if (CREATE_GAME_SESSION == msg_type) {
      LOG_INFO("CreateGameSession received");
      // new connection, so we need to create new ConnectionStorage for it
      struct CreateGameSessionMsg cgs_msg = {0};
      deserialize_cgs(&cgs_msg, buf);

      handle_cgs(&cgs_msg, map, *session_counter++, client_sock);
    }

    // NOTE: there is no need for yoda style in 2020. Any modern compiler will
    //       show a warning if you wrtite if (a = b) { ... }
    if (CONNECT_TO_SESSION == msg_type) {
      LOG_INFO("ConnectToSession message received");

      struct ConnectToSessionMsg cts_msg = {0};
      deserialize_cts(&cts_msg, buf);

      handle_cts(&cts_msg, map, *session_counter, client_sock);
    }
  }
}

void run(char* ip, char* port, struct ConnectionMap* con_map) {
  int session_counter = 0;
  int server_sock = setup_server(ip, port);
  if (-1 == server_sock) return;

  fd_set current_sockets;
  fd_set ready_sockets;

  FD_ZERO(&current_sockets);
  FD_SET(server_sock, &current_sockets);

  // TODO: handle ctrl-c
  while (1) {
    memcpy(&ready_sockets, &current_sockets, sizeof(current_sockets));

    // just read for now
    if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
      LOG_ERROR("select() error: %s", strerror(errno));
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
          handle_connection(i, con_map, &session_counter);
          FD_CLR(i, &current_sockets);
        }
      }
    }
  }
}


