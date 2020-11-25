#include <stdlib.h>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>

#include "pong.h"
#include "log.h"

#define ERR_MESSAGE_SIZE 100

// -1 in case of errors
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


// TODO: for god sake, don't even try to use atoi more
// returns -1 in case of error and fill err_msg
// returns 0 in case of no error, but no further processing is needed
// returns number of params otherwise
static int fill_params(int argc, char* argv[], LaunchParams* params) {
  char default_host[] = "127.0.0.1";
  int default_port = 1337;

  // fill params with default values;
  params->password[0] = '\0';
  params->port = default_port;
  strcpy(params->ip, default_host);
  params->game_mode = LOCAL_GAME;
  params->session_id = -1;

  if (argc == 1) {
    LOG_INFO("Not enough params, needed at least 2, see help");
    return -1;
  }
  // Might be several cases:
  // ./pong help
  // ./pong local
  // ./pong create (to create default session without password on default host)
  if (argc == 2) {
    if (strcmp(argv[1], "help") == 0) {
      LOG_INFO("available options: local, create, connect");
      return 0;
    }
    if (strcmp(argv[1], "local") == 0) {
      // don't need to fill with something
      return argc;
    }

    if (strcmp(argv[1], "create") == 0) {
      params->game_mode = REMOTE_NEW_GAME;
      return argc;
    }

    LOG_ERROR("invalid param: %s, see help", argv[1]);
    return -1;
  }

  // Might be only :
  // ./pong create pw (create session on default server with password)
  // ./pong connect id (connect on default server with no password on id)
  if (argc == 3) {
    if (strcmp(argv[1], "create") == 0) {
      params->game_mode = REMOTE_NEW_GAME;
      if (strlen(argv[2])+1 > MAX_PASSWORD_SIZE) {
        LOG_ERROR("Password is too long, please fit it in %d symbols", MAX_PASSWORD_SIZE-1);
        return -1;
      }
      strcpy(params->password, argv[2]);
      return argc;
    }

    if (strcmp(argv[1], "connect") == 0) {
      params->game_mode = REMOTE_CONNECT_GAME;
      int session_id = parse_uint(argv[2]);

      if (session_id == -1) {
        LOG_ERROR("Invalid session id: %s", argv[2]);
        return -1;
      }
      params->session_id = session_id;
      return argc;
    }

    LOG_ERROR("invalid parameter: %s", argv[1]);
    return -1;

  }

  // Might be in several cases:
  // ./pong create host port // create no password on host
  // ./pong connect id pw // connect to existing session with pw
  if (argc == 4) {
    if (strcmp(argv[1], "create") == 0) {

      if (strlen(argv[2]) > 16) {
        LOG_ERROR("%s is not valid host", argv[2]);
        return -1;
      }

      strcpy(params->ip, argv[2]);

      int port = parse_uint(argv[3]);

      if (port == -1) {
        LOG_ERROR("%s is not valid port", argv[3]);
        return -1;
      }

      params->port = port;
      return argc;
    }

    if (strcmp(argv[1], "connect") == 0) {

      int id = parse_uint(argv[2]);
      if (id == -1) {
        LOG_ERROR("Invalid session id: %s", argv[2]);
        return -1;
      }

      params->session_id = id;

      if (strlen(argv[3])+1 > MAX_PASSWORD_SIZE) {
        LOG_ERROR("Password is too long, please fit it in %d symbols", MAX_PASSWORD_SIZE-1);
        return -1;
      }
      strcpy(params->password, argv[3]);
      params->game_mode = REMOTE_CONNECT_GAME;
      return argc;
    }
  }

  // ./pong connect host port id
  // ./pong connect idp
  if (argc == 5) {
    if (strcmp(argv[1], "connect") != 0) {
      LOG_ERROR("Invalid combination of params");
      return -1;
    }

    if (strlen(argv[2]) > 16) {
      LOG_ERROR("%s is not valid host", argv[2]);
      return -1;
    }

    strcpy(params->ip, argv[2]);

    int port = parse_uint(argv[3]);
    if (port == -1) {
      LOG_ERROR("%s is not valid port", argv[3]);
      return -1;
    }

    params->port = port;

    int id = parse_uint(argv[4]);
    if (id == -1) {
      LOG_ERROR("invalid session id: %s", argv[4]);
      return -1;
    }

    params->session_id = id;
    return argc;
  }

  // ./connect host port id password
  if (argc == 6) {
    if (strcmp(argv[1], "connect") != 0) {
      LOG_ERROR("Invalid combination of params");
      return -1;
    }

    if (strlen(argv[2]) > 16) {
      LOG_ERROR("%s is not valid host", argv[2]);
      return -1;
    }

    strcpy(params->ip, argv[2]);

    int port = parse_uint(argv[3]);

    if (port == -1) {
      LOG_ERROR("%s is not valid port", argv[3]);
      return -1;
    }

    params->port = port;

    int id = parse_uint(argv[4]);
    if (id == -1) {
      LOG_ERROR("id should be at least 0");
      return -1;
    }

    params->session_id = id;
    if (strlen(argv[5])+1 > MAX_PASSWORD_SIZE) {
      LOG_ERROR("Password is too long, please fit it in %d symbols", MAX_PASSWORD_SIZE +1);
      return -1;
    }

    strcpy(params->password, argv[5]);
    return argc;
  }

  LOG_ERROR("invalid arguments");
  return -1;

}

int main(int argc, char* argv[]) {
  LaunchParams params = {0};

  int res = fill_params(argc, argv, &params);

  if (res == 0) {
    return EXIT_SUCCESS;
  }
  if (res == -1) {
    return EXIT_FAILURE;
  }

#ifdef _WIN32
  SDL_SetMainReady();
#endif

  if (SDL_Init(SDL_INIT_VIDEO)) {
    LOG_ERROR("Could not initialize SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_version v;
  SDL_GetVersion(&v);
  LOG_INFO("System info:\n"
           "   OS: %s\n"
           "   SDL %d.%d.%d\n"
           "   %d threads\n"
           "   %dMB RAM",
           SDL_GetPlatform(), v.major, v.minor, v.patch,
           SDL_GetCPUCount(), SDL_GetSystemRAM());

  Pong pong;
  if (pong_init(&pong, &params)) {
    return EXIT_FAILURE;
  }

  pong_run(&pong);
  pong_close(&pong);

  SDL_Quit();
  LOG_INFO("Closed successfully");
  return EXIT_SUCCESS;
}
