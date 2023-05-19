#include "audio_printer.hh"

#include "../common/err.hh"

#include <unistd.h>

AudioPrinterWorker::AudioPrinterWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const int audio_receiver_fd
) : Worker(running), _buffer(buffer), _audio_receiver_fd(audio_receiver_fd) {}

void AudioPrinterWorker::run() {
    while (running) {
        auto lock = _buffer.lock();
        _cv.wait(lock, [&] { return !running || !_wait; });
        if (!running)
            break;

        size_t to_print = _buffer->cnt_upto_gap() * _buffer->psize();
        if (to_print != _buffer->range())
            report_packet_loss();
        else
            _buffer->dump_tail(to_print);
        _wait = true;
    }
}

AudioPrinterWorker::~AudioPrinterWorker() {
    if (-1 == close(_audio_receiver_fd))
        fatal("close");
}

// this should be called under a mutex lock
void AudioPrinterWorker::signal() {
    _wait = false;
    _cv.notify_one();
}

void AudioPrinterWorker::report_packet_loss() const {
    send_msg(_audio_receiver_fd);
}
