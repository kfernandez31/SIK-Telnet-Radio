#pragma once

#include "../common/worker.hh"
#include "../common/event_queue.hh"
#include "../common/circular_buffer.hh"
#include "../common/udp_socket.hh"
#include "../common/synced_ptr.hh"

#include <netinet/in.h>
#include <cstddef>

#include <queue>
#include <chrono>

struct RetransmitterWorker : public Worker {
private:
    SyncedPtr<CircularBuffer> _packet_cache;
    SyncedPtr<std::queue<RexmitRequest>> _job_queue;
    SyncedPtr<EventQueue> _my_event;
    uint64_t _session_id;
    std::chrono::milliseconds _rtime;
    UdpSocket _data_socket;

    void handle_retransmission(RexmitRequest&& req);
public:
    RetransmitterWorker() = delete;
    RetransmitterWorker(
        const volatile sig_atomic_t& running,
        const SyncedPtr<CircularBuffer>& packet_cache,
        const SyncedPtr<std::queue<RexmitRequest>>& job_queue,
        const SyncedPtr<EventQueue>& my_event,
        const uint64_t session_id,
        const std::chrono::milliseconds rtime
    );

    void run() override;
};
