#include "rexmit_sender.hh"

#include "../common/datagram.hh"
#include "../common/err.hh"

#include <thread>
#include <chrono>
#include <optional>

RexmitSenderWorker::RexmitSenderWorker(
    const volatile sig_atomic_t& running, 
    const std::chrono::milliseconds rtime,
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station
)
    : Worker(running)
    , _buffer(buffer)
    , _stations(stations)
    , _current_station(current_station) 
    , _rtime(rtime)
{
    //TODO: socket
}


void RexmitSenderWorker::order_retransmission() {
    auto buffer_lock = _buffer.lock();
    size_t i = _buffer->tail();
    std::vector<uint64_t> packet_ids;
    do {
        if (!_buffer->occupied(i))
            packet_ids.push_back(i + _buffer->abs_tail());
        i = (i + _buffer->psize()) % _buffer->rounded_cap();
    } while (i != _buffer->tail());
    RexmitRequest request(packet_ids);
    std::string request_str = request.to_str();

    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    _ctrl_socket.sendto(request_str.c_str(), request_str.length(), (*_current_station)->ctrl_addr);
}

// This approach is much more efficient than the one described in the task description
void RexmitSenderWorker::run() {
    std::optional<std::chrono::steady_clock::time_point> prev_rexmit_time;
    while (running) {
        sleep_until(prev_rexmit_time, _rtime);
        order_retransmission();
    }
}
