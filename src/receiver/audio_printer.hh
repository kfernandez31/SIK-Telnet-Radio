#pragma once

#include "../common/worker.hh"

#include "../common/circular_buffer.hh"
#include "../common/radio_station.hh"
#include "../common/ptr.hh"

#include <memory>
#include <condition_variable>

struct AudioPrinterWorker : public Worker {
private:
    SyncedPtr<CircularBuffer> _buffer;
    std::condition_variable _cv;
    bool _wait;
    int _audio_receiver_fd;
public:
    AudioPrinterWorker() = delete;
    AudioPrinterWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& buffer,
        const int audio_receiver_fd
    );
    ~AudioPrinterWorker();
    
    void run() override;
    void signal();
    void report_packet_loss() const;
};
