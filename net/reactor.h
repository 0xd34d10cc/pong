#ifndef REACTOR_H
#define REACTOR_H

enum {
  IO_EVENT_READ  = (1 << 0),
  IO_EVENT_WRITE = (1 << 1),
};

typedef struct Reactor {
  int poll;
} Reactor;


typedef struct Evented {
  int fd;
  unsigned events;
} Evented;

typedef struct IOEvent {
  // A set of events
  unsigned events;
  // IO object for which |events| were triggered
  Evented* object;
} IOEvent;

int reactor_init(Reactor* reactor);
void reactor_close(Reactor* reactor);

// Add IO |object| to |reactor| and subscribe it to |events|
int reactor_register(Reactor* reactor, Evented* object, unsigned events);
// Update |events| subscription for IO |object|
int reactor_update(Reactor* reactor, Evented* object, unsigned events);
// Remove IO |object| from |reactor|
int reactor_deregister(Reactor* reactor, Evented* object);

// Poll |reactor| for |events|
int reactor_poll(Reactor* reactor, IOEvent* events, int n_events, int timeout_ms);

#endif // REACTOR_H
