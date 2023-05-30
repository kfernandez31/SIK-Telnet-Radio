#include "rexmit_sender.hh"

#include "../common/datagram.hh"

#include <thread>
#include <chrono>

using namespace std::chrono;

RexmitSenderWorker::RexmitSenderWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const std::chrono::milliseconds rtime
)
    : Worker(running, "RexmitSender")
    , _buffer(buffer)
    , _stations(stations)
    , _current_station(current_station) 
    , _rtime(rtime)
    {}

// notes:
// - this turns out to be a more efficient approach 
//   than the one described in the task description
// - here we assume that the request will fit 
//   in a UDP packet and sendto will succeed
void RexmitSenderWorker::order_retransmission() {
    auto stations_lock        = _stations.lock();   
    auto current_station_lock = _current_station.lock();
    if (*_current_station == _stations->end()) {
        log_debug("[%s] no station playing", name.c_str());
        return; // not connected to any station => no one to ask for retransmission
    }
    log_debug("[%s] gathering missing packets...", name.c_str());

    auto buffer_lock = _buffer.lock();
    size_t i = _buffer->tail();
    std::vector<uint64_t> packet_ids;
    log_debug("[%s] tail = %zu, head = %zu", name.c_str(), _buffer->tail(), _buffer->head());
    while (i != _buffer->head()) {
        log_debug("[%s] i = %zu", name.c_str(), i);
        log_debug("[%s] abs_tail = %zu", name.c_str(), _buffer->abs_tail());
        if (!_buffer->occupied(i)) {
            log_debug("[%s] pushing!!!!");
            packet_ids.push_back(i + _buffer->abs_tail());
        }
        i = (i + _buffer->psize()) % _buffer->rounded_cap();
    } 

    if (packet_ids.size() > 0) {
        try {
            RexmitRequest request(packet_ids);
            std::string request_str = request.to_str();
            log_debug("[%s] sending request: %s", name.c_str(), request_str.c_str());
            _ctrl_socket.sendto(request_str.c_str(), request_str.length(), (*_current_station)->ctrl_addr);
        } catch(const std::exception& e) {
            fatal("[%s] malformed rexmit request: %s", name.c_str());
        }
    } else {
        log_debug("[%s] nothing to request", name.c_str());
    }
}

void RexmitSenderWorker::run() {
    steady_clock::time_point prev_sleep = steady_clock::now();
    while (running) {
        std::this_thread::sleep_until(prev_sleep + _rtime);
        prev_sleep = steady_clock::now();
        order_retransmission();
    }
}
