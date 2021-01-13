#include "args.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

static const char* DEFAULT_HOST = "127.0.0.1";
static const int DEFAULT_PORT = 1337;
static const char* GAME_MODES = "local, create, join";

void args_init(Args* params) {

  // fill params with default values;
  params->password[0] = '\0';
  params->port = DEFAULT_PORT;
  strcpy(params->ip, DEFAULT_HOST);
  params->game_mode = LOCAL_GAME;
  params->lobby_id = -1;
}


static int parse_uint(const char* str) {
  char* end;
  errno = 0;

  int res = strtoul(str, &end, 10);
  if (errno != 0) {
    return -1;
  }

  if (end[0] != '\0') {
    return -1;
  }

  return res;
}


static bool check_flag(const char* arg, const char* short_flag, const char* long_flag) {
    return strcmp(arg, short_flag) == 0 || 
           strcmp(arg, long_flag) == 0;
}


static bool log_help(char* error, unsigned int size) {
    snprintf(error, size,
             "Usage: pong [flags]\n\n"
             "--help, -h                       get help information\n"
             "--mode, -m [%s] start game locally\n"
             "--password                       lobby's password to join or create\n"
             "--ip                             server ip. (default - %s)\n"
             "--port                           server port. (default - %d)\n"
             "--lobby-id                       lobby's id\n",
             GAME_MODES, DEFAULT_HOST, DEFAULT_PORT);
    
    return false;
}


static bool log_empty_value(char* error, unsigned int size, const char* flag, const char* expected) {
  snprintf(error, size, "No value for flag %s. Expected: %s\n", flag, expected);
  return false;
}


static bool log_unexpected_value(char* error, unsigned int size, const char* flag, const char* actual, const char* expected) {
  snprintf(error, size, "Unexpected value for flag %s: %s. Expected: %s\n", flag, actual, expected);
  return false;
}


bool args_parse(Args* params, char** args, char* error, unsigned int size) {
  for(char** curr = args; *curr != NULL; ++curr) {
    char* flag = *curr;
    if (check_flag(flag, "-h", "--help")) {
        return log_help(error, size);
    }
    else if (check_flag(flag, "-m", "--mode")) {
      ++curr;
      char* value = *curr;
      if (value == NULL) {
        return log_empty_value(error, size, flag, GAME_MODES);
      }
      
      if (strcmp(value, "local") == 0) {
        params->game_mode = LOCAL_GAME;
      }
      else if (strcmp(value, "create") == 0) {
        params->game_mode = REMOTE_NEW_GAME;
      }
      else if (strcmp(value, "join") == 0) {
        params->game_mode = REMOTE_CONNECT_GAME;
      }
      else {
        log_unexpected_value(error, size, flag, value, GAME_MODES);
        return false;
      }
    }
    else if (check_flag(flag, "", "--password")) {
      ++curr;
      if(*curr == NULL) {
        return log_empty_value(error, size, flag, "password");
      }

      char* password = *curr;

      if(strlen(password) + 1 > sizeof(params->password)) {
        char buff[128];
        snprintf(buff, sizeof(buff), "password length should be less than %zu", sizeof(params->password) - 1);
        return log_unexpected_value(error, size, flag, password, buff);
      }

      strcpy(params->password, password);
    }
    else if (check_flag(flag, "", "--ip")) {
      ++curr;
      if(*curr == NULL) {
        return log_empty_value(error, size, flag, "ip");
      }

      char* ip = *curr;
      if(strlen(ip) + 1 > sizeof(params->ip)) {
        char buff[128];
        snprintf(buff, sizeof(buff), "ip length should be less than %zu", sizeof(params->password) - 1);
        return log_unexpected_value(error, size, flag, ip, buff);
      }

      if(inet_addr(ip) == INADDR_NONE) {
        return log_unexpected_value(error, size, flag, ip, "valid ip address");
      }

      strcpy(params->ip, ip);
    }
    else if (check_flag(flag, "", "--port")) {
      ++curr;
      if(*curr == NULL) {
        return log_empty_value(error, size, flag, "port");
      }

      int port = parse_uint(*curr);
      if(port < 0 || port >= 1<<16) {
        return log_unexpected_value(error, size, flag, *curr, "valid port number");
      }

      params->port = port;
    }
    else if (check_flag(flag, "", "--lobby-id")) {
      ++curr;
      if(*curr == NULL) {
        return log_empty_value(error, size, flag, "lobby-id");
      }

      int lobby_id = parse_uint(*curr);
      if(lobby_id == -1) {
        return log_unexpected_value(error, size, flag, *curr, "valid lobby id");
      }

      params->lobby_id = lobby_id;
    }
    else {
      snprintf(error, size, "Unexpected flag %s. See --help to get detailed info\n", flag);
      return false;
    }
  }

  return true;
}