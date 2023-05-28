#pragma once

#include "../common/event_queue.hh"
#include "../common/worker.hh"
#include "../common/synced_ptr.hh"
#include "../common/circular_buffer.hh"
#include "../common/radio_station.hh"

#include <chrono>

struct RexmitSenderWorker : public Worker {
private:
    SyncedPtr<CircularBuffer> _buffer;
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    UdpSocket _ctrl_socket;
    std::chrono::milliseconds _rtime;

    void order_retransmission();
public:
    RexmitSenderWorker() = delete;
    RexmitSenderWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& buffer,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const std::chrono::milliseconds rtime
    );

    void run() override;
};
