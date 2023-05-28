#include "event_queue.hh"

#include "err.hh"
#include <unistd.h>

#include "../common/synced_ptr.hh"
#include "../common/worker.hh"

EventQueue::EventQueue() {
    if (-1 == pipe(_fds))
        fatal("pipe");
}

EventQueue::~EventQueue() {
    if (-1 == close(_fds[STDIN_FILENO]))
        fatal("close");
    if (-1 == close(_fds[STDOUT_FILENO]))
        fatal("close");
}

int EventQueue::in_fd() const {
    return _fds[STDIN_FILENO];
}

void EventQueue::push(const EventQueue::EventType event_type) {
    if (-1 == write(_fds[STDOUT_FILENO], &event_type, sizeof(event_type)))
        fatal("write");
}

EventQueue::EventType EventQueue::pop() const {
    EventQueue::EventType event_type;
    if (-1 == read(_fds[STDIN_FILENO], &event_type, sizeof(event_type)))
        fatal("read");
    return event_type;
}
