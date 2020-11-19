#include <stdlib.h>

#ifdef _WIN32
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>

#include "pong.h"
#include "log.h"


int main(int argc, char* argv[]) {
  if (argc > 3) {
    LOG_ERROR("Unexpected number of arguments: got %d, expected 2", argc - 1);
    return EXIT_FAILURE;
  }

  const char* host = argc < 2 ? "127.0.0.1" : argv[1];
  if (strlen(host) > 16) {
    LOG_ERROR("Invalid ip address: %s", host);
    return EXIT_FAILURE;
  }

  unsigned short port = 1337;
  if (argc == 3) {
    port = (unsigned short)atoi(argv[2]);

    if (port == 0) {
      LOG_ERROR("%s is not a valid port number", argv[2]);
      return EXIT_FAILURE;
    }
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
  if (pong_init(&pong, host, port)) {
    return EXIT_FAILURE;
  }

  pong_run(&pong);
  pong_close(&pong);

  SDL_Quit();
  LOG_INFO("Closed successfully");
  return EXIT_SUCCESS;
}
