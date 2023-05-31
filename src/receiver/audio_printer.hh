#pragma once

#include "../common/event_queue.hh"
#include "../common/worker.hh"
#include "../common/circular_buffer.hh"
#include "../common/radio_station.hh"
#include "../common/synced_ptr.hh"

#include <memory>

struct AudioPrinterWorker : public Worker {
private:
    SyncedPtr<CircularBuffer> _buffer;
    SyncedPtr<EventQueue> _my_event;

    void handle_print();
public:
    AudioPrinterWorker() = delete;
    AudioPrinterWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& buffer,
        const SyncedPtr<EventQueue>& my_event
    );

    void run() override;
};
