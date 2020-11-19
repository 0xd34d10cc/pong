#include "reactor.h"

#include <errno.h>
#include <string.h>

#include <sys/epoll.h>

int reactor_init(Reactor* reactor) {
  int poll = epoll_create(1);
  if (poll == -1) {
    return -1;
  }

  reactor->poll = poll;
  return 0;
}

int reactor_register(Reactor* reactor, int fd, void* data, unsigned events) {
  unsigned poll_events = 0;

  if (events & IO_EVENT_READ) {
    poll_events |= EPOLLIN;
  }
  if (events & IO_EVENT_WRITE) {
    poll_events |= EPOLLOUT;
  }

  struct epoll_event event;
  event.events = poll_events;
  event.data.ptr = data;

  if (epoll_ctl(reactor->poll, EPOLL_CTL_ADD, fd, &event) == -1) {
    return -1;
  }

  return 0;
}

int reactor_deregister(Reactor* reactor, int fd) {
  if (epoll_ctl(reactor->poll, EPOLL_CTL_DEL, fd, NULL) == -1) {
    return -1;
  }
  return 0;
}

int reactor_update(Reactor* reactor, int fd, void* data, unsigned events) {
  unsigned poll_events = 0;

  if (events & IO_EVENT_READ) {
    poll_events |= EPOLLIN;
  }
  if (events & IO_EVENT_WRITE) {
    poll_events |= EPOLLOUT;
  }

  struct epoll_event event;
  event.events = poll_events;
  event.data.ptr = data;

  if (epoll_ctl(reactor->poll, EPOLL_CTL_MOD, fd, &event) == -1) {
    return -1;
  }

  return 0;
}

static const int MAX_EVENTS = 64;

// TODO: Handle case when io_events_size is bigger than MAX_EVENTS
int reactor_poll(Reactor* reactor, IOEvent* io_events, int io_events_size, int timeout_ms) {
  struct epoll_event epoll_events[MAX_EVENTS];

  int max_events = io_events_size > MAX_EVENTS ? MAX_EVENTS : io_events_size;

  int n_events = epoll_wait(reactor->poll, epoll_events, max_events, timeout_ms);

  if (n_events == -1) {
    return -1;
  }

  for (int i = 0; i < n_events; i++) {
    unsigned poll_events = io_events[i].events;
    unsigned events = 0;

    if (poll_events & EPOLLIN) {
      events |= IO_EVENT_READ;
    }
    if (poll_events & EPOLLOUT) {
      events |= IO_EVENT_WRITE;
    }

    io_events[i].events = events;
    io_events[i].data = epoll_events[i].data.ptr;
  }

  return n_events;
}