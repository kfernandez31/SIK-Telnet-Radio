#include "audio_printer.hh"

#include <unistd.h>
#include <poll.h>

#define MY_EVENT    0
#define NUM_POLLFDS 1

AudioPrinterWorker::AudioPrinterWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<EventQueue>& my_event
)
    : Worker(running, "AudioPrinter")
    , _buffer(buffer)
    , _my_event(my_event)
    {}

void AudioPrinterWorker::handle_print() {
    auto buf_lock = _buffer.lock();
    // note: we don't notify AudioReceiver and he doesn't reset the session. 
    // This allows for a much better user experience, less choppy sound, just occasional silence
    if (!_buffer->occupied(_buffer->tail())) 
        log_warn("[%s] detected packet loss!", name.c_str());
    _buffer->dump_tail(_buffer->psize());
}

void AudioPrinterWorker::run() {
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd = _my_event->in_fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    while (running) {
        if (-1 == poll(poll_fds, NUM_POLLFDS, -1))
            fatal("poll");

        if (poll_fds[MY_EVENT].revents & POLLIN) {
            poll_fds[MY_EVENT].revents = 0;
            EventQueue::EventType event_val = _my_event->pop();
            switch (event_val) {
                case EventQueue::EventType::TERMINATE:
                    return;
                case EventQueue::EventType::NEW_JOBS:
                    handle_print();
                default: break;
            }
        }
    }

    log_debug("[%s] going down", name.c_str());
}
