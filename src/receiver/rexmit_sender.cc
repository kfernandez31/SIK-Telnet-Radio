#include "rexmit_sender.hh"

#include "../common/datagram.hh"
#include "../common/err.hh"

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
    : Worker(running)
    , _buffer(buffer)
    , _stations(stations)
    , _current_station(current_station) 
    , _rtime(rtime)
    {}

// This approach is much more efficient than the one in the task description
void RexmitSenderWorker::order_retransmission() {
    auto stations_lock        = _stations.lock();   
    auto current_station_lock = _current_station.lock();
    if (*_current_station == _stations->end())
        return; // not connected to any station => no one to ask for retransmission

    auto buffer_lock = _buffer.lock();
    size_t i = _buffer->tail();
    std::vector<uint64_t> packet_ids;
    do {
        if (!_buffer->occupied(i))
            packet_ids.push_back(i + _buffer->abs_tail());
        i = (i + _buffer->psize()) % _buffer->rounded_cap();
    } while (i != _buffer->tail());

    if (packet_ids.size() > 0) {
        RexmitRequest request(packet_ids);
        std::string request_str = request.to_str();
        _ctrl_socket.sendto(request_str.c_str(), request_str.length(), (*_current_station)->get_ctrl_addr());
    }
}

void RexmitSenderWorker::run() {
    steady_clock::time_point prev_sleep = steady_clock::now();
    while (running) {
        std::this_thread::sleep_until(prev_sleep + _rtime);
        prev_sleep = std::chrono::steady_clock::now();
        order_retransmission();
    }
}
