#include "event_queue.hh"

#include "log.hh"
#include <unistd.h>

#include "../common/synced_ptr.hh"
#include "../common/worker.hh"

EventQueue::EventQueue() {
    if (pipe(_fds)) == -1)
        fatal("pipe");
}

EventQueue::~EventQueue() {
    for (auto i : {STDIN_FILENO, STDOUT_FILENO})
        if (close(_fds[i])) == -1)
            fatal("close");
}

int EventQueue::in_fd() const {
    return _fds[STDIN_FILENO];
}

void EventQueue::push(const EventQueue::EventType event_type) {
    if (write(_fds[STDOUT_FILENO], &event_type, sizeof(event_type))) == -1)
        fatal("write");
}

EventQueue::EventType EventQueue::pop() const {
    EventQueue::EventType event_type;
    if (read(_fds[STDIN_FILENO], &event_type, sizeof(event_type))) == -1)
        fatal("read");
    return event_type;
}
