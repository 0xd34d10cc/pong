#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#include "pong.h"
#include "log.h"


int main(int argc, char* argv[]) {
  (void)argc;
  Args params;
  args_init(&params);

  char error[1024];
  if(!args_parse(&params, argv+1, error, sizeof(error))) {
    fprintf(stderr, "%s\n", error);
    return EXIT_FAILURE;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS)) {
    LOG_ERROR("Could not initialize SDL: %s", SDL_GetError());
    return EXIT_FAILURE;
  }

  LOG_INFO("Arguments:\n"
           "   Mode: %d\n"
           "   IP: %s\n"
           "   Port: %d\n"
           "   Lobby id: %d\n"
           "   Password: %s",
           params.game_mode, params.ip,
           params.port, params.lobby_id,
           params.password);

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
    LOG_ERROR("Initialization failed");
    return EXIT_FAILURE;
  }

  pong_run(&pong);
  pong_close(&pong);

  SDL_Quit();
  LOG_INFO("Closed successfully");
  return EXIT_SUCCESS;
}
