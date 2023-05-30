#include "audio_printer.hh"

#include <unistd.h>
#include <poll.h>

#define MY_EVENT    0
#define NUM_POLLFDS 1

AudioPrinterWorker::AudioPrinterWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<EventQueue>& my_event,
    const SyncedPtr<EventQueue>& audio_receiver_event
)
    : Worker(running, "AudioPrinter")
    , _buffer(buffer)
    , _my_event(my_event)
    , _audio_receiver_event(audio_receiver_event)
    {}

void AudioPrinterWorker::handle_print() {
    auto buf_lock = _buffer.lock();
    size_t to_print = _buffer->cnt_upto_gap() * _buffer->psize();
    if (to_print == _buffer->range())
        _buffer->dump_tail(to_print);
    else {
        log_debug("[%s] detected packet loss!", name.c_str());
        auto event_lock = _audio_receiver_event.lock();
        _audio_receiver_event->push(EventQueue::EventType::PACKET_LOSS);
    }
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
                case EventQueue::EventType::NEW_JOBS: {
                    log_debug("[%s] got new jobs!", name.c_str());
                    handle_print();
                }
                default: break;
            }
        }
    }
}
