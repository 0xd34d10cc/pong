#include "pong.h"
#include "log.h"
#include "messages.h"

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>


int pong_init(Pong* pong, const char* ip, unsigned short port) {
  pong->window = SDL_CreateWindow(
      "pong",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      DEFAULT_WINDOW_WIDTH,
      DEFAULT_WINDOW_HEIGHT,
      SDL_WINDOW_SHOWN
  );

  if (!pong->window) {
    LOG_ERROR("Failed to create window: %s", SDL_GetError());
    return -1;
  }

  if (renderer_init(&pong->renderer, pong->window)) {
    return -1;
  }

  game_init(&pong->game, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

  if (reactor_init(&pong->reactor)) {
    LOG_ERROR("Can't initialize reactor");
    return -1;
  }

  pong->connection_state.state = DISCONNECTED;
  strcpy(pong->connection_state.ip, ip);
  pong->connection_state.port = port;

  network_session_init(&pong->network_session);

  pong->game_session.id = -1;
  pong->game_session.state = NOT_IN_SESSION;
  memset(pong->game_session.opponent_ip, 0, sizeof(pong->game_session.opponent_ip));

  reactor_register(&pong->reactor, pong->network_session.socket, &pong->network_session, 0);
  pong->running = true;
  return 0;
}

void pong_close(Pong* pong) {
  renderer_close(&pong->renderer);
  SDL_DestroyWindow(pong->window);
}

static void pong_event(Pong* pong, SDL_Event* event) {
  switch (event->type) {
    case SDL_QUIT:
      pong->running = false;
      break;
    case SDL_KEYDOWN:
      switch (event->key.keysym.scancode) {
        case SDL_SCANCODE_Q:
          if (SDL_GetModState() & KMOD_LCTRL) {
            pong->running = false;
          }
          break;
        case SDL_SCANCODE_RETURN:
          game_event(&pong->game, EVENT_RESTART);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

static void pong_process_events(Pong* pong) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    pong_event(pong, &event);
  }

  const Uint8* kb = SDL_GetKeyboardState(NULL);
  if (kb[SDL_SCANCODE_LEFT] || kb[SDL_SCANCODE_A]) {
    game_event(&pong->game, EVENT_MOVE_LEFT);
  }
  else if (kb[SDL_SCANCODE_RIGHT] || kb[SDL_SCANCODE_D]) {
    game_event(&pong->game, EVENT_MOVE_RIGHT);
  }
}

const static int EVENTS_MAX_SIZE = 64;

static int start_connection(Pong* pong) {
  int connected = network_session_connect(&pong->network_session,
                                          pong->connection_state.ip,
                                          pong->connection_state.port);
  if (connected == -1) {
    LOG_ERROR("Can't connect to the game server with addres: %s:%d with error: %s",
        pong->connection_state.ip,
        pong->connection_state.port,
        strerror(errno));
    return -1;
  }

  int update_res = reactor_update(&pong->reactor,
                 pong->network_session.socket,
                 &pong->network_session,
                 IO_EVENT_WRITE);

  if (update_res == -1) {
    LOG_ERROR("Can't update io event subscription: %s", strerror(errno));
    return -1;
  }

  pong->connection_state.state = AWAITING_CONNECTION;

  return 0;
}

static int wait_connection(Pong* pong, int timeout_ms) {
  IOEvent io_events[EVENTS_MAX_SIZE];
  int n_events = reactor_poll(&pong->reactor, io_events, EVENTS_MAX_SIZE, timeout_ms);
  if (n_events == -1) {
    LOG_ERROR("reactor_poll failed");
    return -1;
  }

  for (int i = 0; i < n_events; i++) {
    NetworkSession* event_session = (NetworkSession*)io_events[i].data;
    if ((io_events[i].events & IO_EVENT_WRITE) &&
        (event_session == &pong->network_session)) {
      int err = 0;
      socklen_t err_size = sizeof(err);
      if (getsockopt(pong->network_session.socket, SOL_SOCKET, SO_ERROR, &err, &err_size) == -1) {
        LOG_ERROR("Can't retrive socket options: %s", strerror(errno));
        return -1;
      }

      if (err != 0) {
        LOG_ERROR("Connection error: %s", strerror(err));
        return -1;
      }

      pong->connection_state.state = CONNECTED;
      break;
    }
  }
  return 0;
}

static int pong_process_game_session(Pong* pong) {
  ClientMessage msg = {0};
  switch (pong->game_session.state) {
    case NOT_IN_SESSION:
      msg.id = CREATE_SESSION;
      msg.create_session.password[0] = 0;

      int res = client_message_write(&msg,
                           pong->network_session.output,
                           sizeof(pong->network_session.output) - pong->network_session.to_send);

      break;

  }
  return 0;
}

static int pong_process_network(Pong* pong, int timeout_ms) {
  unsigned curr_time = SDL_GetTicks();
  unsigned deadline = curr_time + timeout_ms;

  while(curr_time < deadline) {
    unsigned left = deadline - curr_time;
    switch (pong->connection_state.state) {
      case LOCAL:
        SDL_Delay(left);
        break;
      case DISCONNECTED:
        if (start_connection(pong) == -1) {
          return -1;
        }
        break;
      case AWAITING_CONNECTION:
        if (wait_connection(pong, left) == -1) {
          return -1;
        }
       break;
      case CONNECTED:

        //pong_process_game_session(pong);

        SDL_Delay(left);
        break;
    }

    curr_time = SDL_GetTicks();
  }
  return 0;
}

void pong_run(Pong* pong) {
  while (pong->running) {
    // process events
    game_step_begin(&pong->game);
    pong_process_events(pong);
    game_step_end(&pong->game, 16 /* ms */);

    // render game state
    renderer_render(&pong->renderer, &pong->game);

    // wait for next frame
    pong_process_network(pong, 16);
  }
}
