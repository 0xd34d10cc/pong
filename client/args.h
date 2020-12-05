#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include "game/protocol.h"
#include "bool.h"

typedef enum GameMode {
  LOCAL_GAME = 0,
  REMOTE_NEW_GAME,
  REMOTE_CONNECT_GAME
} GameMode;

typedef struct Args {
  GameMode game_mode;
  char ip[16];
  int port;
  int lobby_id;
  char password[MAX_PASSWORD_SIZE];
} Args;

void args_init(Args* params);

bool args_parse(Args* params, char** args, char* error_buff, unsigned int size);

#endif // ARGUMENT_PARSER_H