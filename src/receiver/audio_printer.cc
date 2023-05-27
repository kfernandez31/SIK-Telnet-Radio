#include "audio_printer.hh"

#include "../common/err.hh"

#include <unistd.h>

AudioPrinterWorker::AudioPrinterWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<EventPipe>& audio_recvr_event
)
    : Worker(running)
    , _buffer(buffer)
    , _audio_recvr_event(audio_recvr_event) 
    {}

void AudioPrinterWorker::run() {
    while (running) {
        auto buf_lock = _buffer.lock();
        _cv.wait(buf_lock, [&] { return !running || !_wait; });
        if (!running)
            break;

        size_t to_print = _buffer->cnt_upto_gap() * _buffer->psize();
        if (to_print == _buffer->range())
            _buffer->dump_tail(to_print);
        else {
            auto event_lock = _audio_recvr_event.lock();
            _audio_recvr_event->set_event(EventPipe::EventType::PACKET_LOSS);
        }
            
        _wait = true;
    }
}

// this should be called under a mutex lock
void AudioPrinterWorker::signal() {
    _wait = false;
    _cv.notify_one();
}
