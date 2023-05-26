#pragma once

#include "../common/worker.hh"
#include "../common/circular_buffer.hh"
#include "../common/udp_socket.hh"
#include "../common/synced_ptr.hh"

#include <netinet/in.h>
#include <cstddef>

#include <mutex>
#include <queue>
#include <chrono>
#include <memory>

struct RetransmitterWorker : public Worker {
private:
    SyncedPtr<CircularBuffer> _packet_cache;
    SyncedPtr<std::queue<RexmitRequest>> _job_queue;
    std::condition_variable _job_queue_cv;
    uint64_t _session_id;
    std::chrono::milliseconds _rtime;
    UdpSocket _data_socket;
    bool _wait;

    void handle_retransmission(RexmitRequest& req);
public:
    RetransmitterWorker() = delete;
    RetransmitterWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& packet_cache,
        const SyncedPtr<std::queue<RexmitRequest>>& job_queue,
        const uint64_t session_id,
        const std::chrono::milliseconds rtime
    );

    void emplace_job(const char* buf);
    void run() override;
};
