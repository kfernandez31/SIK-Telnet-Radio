#include "event_pipe.hh"


#include "../common/synced_ptr.hh"
#include "../common/worker.hh"

EventPipe::EventPipe() {
    if (-1 == pipe(_fds))
        fatal("pipe");
}

EventPipe::~EventPipe() {
    if (-1 == close(_fds[STDIN_FILENO]))
        fatal("close");
    if (-1 == close(_fds[STDOUT_FILENO]))
        fatal("close");
}

int EventPipe::in_fd() const {
    return _fds[STDIN_FILENO];
}

void EventPipe::set_event(const EventPipe::EventType event_type) {
    if (-1 == write(STDOUT_FILENO, &event_type, sizeof(event_type)))
        fatal("write");
}

EventPipe::EventType EventPipe::get_event() const {
    EventPipe::EventType event_type;
    if (-1 == read(STDOUT_FILENO, &event_type, sizeof(event_type)))
        fatal("read");
    return event_type;
}
