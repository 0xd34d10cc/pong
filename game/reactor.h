#ifndef REACTOR_H
#define REACTOR_H

enum {
  IO_EVENT_READ = (1 << 0),
  IO_EVENT_WRITE = (1 << 1)
};

typedef struct Reactor {
  int poll;
} Reactor;

typedef struct IOEvent {
  unsigned events;
  void* data;
} IOEvent;

int reactor_init(Reactor* reactor);

int reactor_register(Reactor* reactor, int fd, void* data, unsigned events);
int reactor_deregister(Reactor* reactor, int fd);
int reactor_update(Reactor* reactor, int fd, void* data, unsigned events);
int reactor_poll(Reactor* reactor, IOEvent* io_events, int io_events_size, int timeout_ms);

#endif // REACTOR_H