#pragma once

#include "../common/event_pipe.hh"
#include "../common/worker.hh"
#include "../common/circular_buffer.hh"
#include "../common/radio_station.hh"
#include "../common/synced_ptr.hh"

#include <memory>
#include <condition_variable>

struct AudioPrinterWorker : public Worker {
private:
    SyncedPtr<CircularBuffer> _buffer;
    SyncedPtr<EventPipe> _audio_recvr_event;
    std::condition_variable _cv;
    bool _wait;
public:
    AudioPrinterWorker() = delete;
    AudioPrinterWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& buffer,
        const SyncedPtr<EventPipe>& audio_recvr_event
    );

    void run() override;
    
    void signal();
};
