#include "retransmitter.hh"

#include "../common/err.hh"
#include "../common/datagram.hh"

#include <thread>
#include <optional>

RetransmitterWorker::RetransmitterWorker(
    const volatile sig_atomic_t& running, 
    const uint64_t session_id,
    const std::chrono::milliseconds rtime,
    const in_port_t data_port,
    const SyncedPtr<CircularBuffer>& packet_cache,
    const SyncedPtr<std::queue<RexmitRequest>>& job_queue
)
    : Worker(running)
    , _wait(false)
    , _session_id(session_id)
    , _rtime(rtime)
    , _packet_cache(packet_cache)
    , _job_queue(job_queue)
{
    //TODO: init socket
}

void RetransmitterWorker::handle_retransmission(RexmitRequest& req) {
    char pkt_buf[TOTAL_PSIZE(_packet_cache->psize())];
    memcpy(pkt_buf, &_session_id, sizeof(_session_id));

    auto lock = _packet_cache.lock();
    std::sort(req.packet_ids.begin(), req.packet_ids.end());

    auto it = req.packet_ids.begin();
    // skip packets before the tail
    while (it != req.packet_ids.end() && *it < _packet_cache->abs_tail())
        ++it;
    if (it == req.packet_ids.end())
        return; // nothing to do
    

    uint64_t cache_packet_num = _packet_cache->abs_tail();
    size_t cache_idx = _packet_cache->tail();
    for (; it != req.packet_ids.end(); ++it) {        
        while (cache_packet_num != _packet_cache->abs_head() && cache_packet_num < *it) {
            cache_packet_num += _packet_cache->psize();
            cache_idx = (cache_idx + _packet_cache->psize()) % _packet_cache->rounded_cap();
        }
        if (cache_packet_num != _packet_cache->abs_head())
            break; // no more packets in cache (theoritically this shouldn't happen)
        memcpy(pkt_buf + sizeof(uint64_t), &cache_packet_num, sizeof(uint64_t));
        memcpy(pkt_buf + 2 * sizeof(uint64_t), _packet_cache->data() + cache_idx, _packet_cache->psize());
        _data_socket.sendto(pkt_buf, sizeof(pkt_buf), req.receiver_addr);
    }
}

void RetransmitterWorker::emplace_job(const char* buf) {
    _job_queue->emplace(buf);
    auto lock = _job_queue.lock();
    _wait = false;
    _job_queue_cv.notify_one();
}

void RetransmitterWorker::run() {
    std::optional<std::chrono::steady_clock::time_point> prev_rexmit_time;
    while (running) {
        size_t new_jobs;
        {
            auto lock = _job_queue.lock();
            _job_queue_cv.wait(lock, [&] { return !running && !wait; });
            if (!running)
                return;
            new_jobs = _job_queue->size();
        }

        sleep_until(prev_rexmit_time, _rtime);

        // intentionally not under a mutex
        while (new_jobs--) {
            RexmitRequest req = _job_queue->front();
            _job_queue->pop();
            handle_retransmission(req);
        }
    }
}
