#include "retransmitter.hh"

#include "../common/datagram.hh"

#include <poll.h>

#include <thread>
#include <optional>
#include <sstream>

using namespace std::chrono;

#define MY_EVENT    0
#define NUM_POLLFDS 1

RetransmitterWorker::RetransmitterWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& packet_cache,
    const SyncedPtr<std::queue<RexmitRequest>>& job_queue,
    const SyncedPtr<EventQueue>& my_event,
    const uint64_t session_id,
    const std::chrono::milliseconds rtime
)
    : Worker(running, "Retransmitter")
    , _packet_cache(packet_cache)
    , _job_queue(job_queue)
    , _my_event(my_event)
    , _session_id(session_id)
    , _rtime(rtime)
    {}

void RetransmitterWorker::handle_retransmission(RexmitRequest&& req) {
    char pkt_buf[TOTAL_PSIZE(_packet_cache->psize())];
    memcpy(pkt_buf, &_session_id, sizeof(_session_id));

    std::sort(req.packet_ids.begin(), req.packet_ids.end());

    auto it = req.packet_ids.begin();
    // skip packets before the tail
    auto lock = _packet_cache.lock();
    while (it != req.packet_ids.end() && *it < _packet_cache->abs_tail())
        ++it;
    if (it == req.packet_ids.end())
        return; // nothing to do
    

    std::vector<uint64_t> retransmitted_ids;
    // this could have been optimized by copying the buffer or somehow locking it in the loop
    uint64_t cache_packet_num = _packet_cache->abs_tail();
    size_t cache_idx = _packet_cache->tail();
    for (; it != req.packet_ids.end(); ++it) {        
        while (cache_packet_num != _packet_cache->abs_head() && cache_packet_num < *it) {
            cache_packet_num += _packet_cache->psize();
            cache_idx = (cache_idx + _packet_cache->psize()) % _packet_cache->rounded_cap();
        }
        if (cache_packet_num != _packet_cache->abs_head()) 
            break; // no more packets in cache
        memcpy(pkt_buf + sizeof(uint64_t), &cache_packet_num, sizeof(uint64_t));
        memcpy(pkt_buf + 2 * sizeof(uint64_t), _packet_cache->data() + cache_idx, _packet_cache->psize());
        if (sizeof(pkt_buf) != _data_socket.sendto(pkt_buf, sizeof(pkt_buf), req.receiver_addr))
            fatal("[%s], packet retransmission failed", name.c_str());
        retransmitted_ids.push_back(cache_packet_num);
    }


    std::ostringstream oss;
    oss << "[";
    if (!retransmitted_ids.empty()) {
        oss << retransmitted_ids.front();
        for (size_t i = 1; i < retransmitted_ids.size(); ++i)
            oss << ", " << retransmitted_ids[i];
    }
    oss << "]";
    log_info("[%s] retransmitted packets : %s", name.c_str(), oss.str().c_str());
}

void RetransmitterWorker::run() {
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd = _my_event->in_fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    steady_clock::time_point prev_sleep = steady_clock::now();
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
                    size_t new_jobs;
                    {
                        auto lock = _job_queue.lock();
                        new_jobs = _job_queue->size();
                    }
                    std::this_thread::sleep_until(prev_sleep + _rtime);
                    prev_sleep = steady_clock::now();

                    while (new_jobs--) {
                        handle_retransmission(std::move(_job_queue->front()));
                        _job_queue->pop();
                    }
                }

                default: break;
            }
        }
    }
}
