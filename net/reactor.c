#include "reactor.h"

#include <errno.h>
#include <string.h>
#include <stdbool.h>

#ifndef WIN32
#include <sys/epoll.h>
#include <unistd.h>
#else 
#include "3rdParty/wepoll.h"
#endif

int reactor_init(Reactor* reactor) {
  int poll = epoll_create(1);
  if (poll == -1) {
    return -1;
  }

  reactor->poll = poll;
  return 0;
}

void reactor_close(Reactor* reactor) {
#ifdef WIN32
  epoll_close(reactor->poll);
#else
  close(reactor->poll);
#endif // WIN32
}

// Checks if |object| is already subscribed to |events|
static bool is_subscribed(Evented* object, unsigned events) {
  return object->events == events;
}

// Convert reactor events to epoll events
static unsigned to_epoll(unsigned events) {
#ifndef WIN32
  unsigned epoll_events = EPOLLET;
#else
  unsigned epoll_events = 0;
#endif

  if (events & IO_EVENT_READ) {
    epoll_events |= EPOLLIN;
  }

  if (events & IO_EVENT_WRITE) {
    epoll_events |= EPOLLOUT;
  }

  return epoll_events;
}

// Convert epoll events to reactor events
static unsigned to_reactor(unsigned epoll_events) {
  unsigned events = 0;

  if (epoll_events & EPOLLIN) {
    events |= IO_EVENT_READ;
  }

  if (epoll_events & EPOLLOUT) {
    events |= IO_EVENT_WRITE;
  }

  return events;
}

int reactor_register(Reactor* reactor, Evented* object, unsigned events) {
  struct epoll_event event;
  event.events = to_epoll(events);
  event.data.ptr = object;
  if (epoll_ctl(reactor->poll, EPOLL_CTL_ADD, object->fd, &event) == -1) {
    return -1;
  }

  object->events = events;
  return 0;
}

int reactor_update(Reactor* reactor, Evented* object, unsigned events) {
  if (is_subscribed(object, events)) {
    return 0;
  }

  struct epoll_event event;
  event.events = to_epoll(events);
  event.data.ptr = object;
  if (epoll_ctl(reactor->poll, EPOLL_CTL_MOD, object->fd, &event) == -1) {
    return -1;
  }

  object->events = events;
  return 0;
}

int reactor_deregister(Reactor* reactor, Evented* object) {
  return epoll_ctl(reactor->poll, EPOLL_CTL_DEL, object->fd, NULL);
}

#define MAX_EVENTS 64
int reactor_poll(Reactor* reactor, IOEvent* events, int n_events, int timeout_ms) {
  struct epoll_event epoll_events[MAX_EVENTS];
  // TODO: handle this case properly
  if (n_events > MAX_EVENTS) {
    n_events = MAX_EVENTS;
  }

  int n = epoll_wait(reactor->poll, epoll_events, n_events, timeout_ms);
  if (n == -1) {
    return -1;
  }

  for (int i = 0; i < n; ++i) {
    events[i].events = to_reactor(epoll_events[i].events);
    events[i].object = epoll_events[i].data.ptr;
  }

  return n;
}
