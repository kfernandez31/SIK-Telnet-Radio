#include "rexmit_sender.hh"

#include "../common/datagram.hh"

#include <thread>
#include <chrono>

using namespace std::chrono;

static const seconds SENDING_TIMEOUT = seconds(1);

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
{
    _ctrl_socket.set_sending_timeout(SENDING_TIMEOUT.count());
}

// notes:
// - this turns out to be a more efficient approach 
//   than the one described in the task description
// - here we assume that the request will fit 
//   in a UDP packet
void RexmitSenderWorker::order_retransmission() {
    auto stations_lock        = _stations.lock();   
    auto current_station_lock = _current_station.lock();
    if (*_current_station == _stations->end()) 
        return; // not connected to any station => no one to ask for retransmission
    
    auto buffer_lock = _buffer.lock();
    size_t i = _buffer->tail();
    std::vector<uint64_t> packet_ids;

    if (!_buffer->empty()) {
        while (i != _buffer->head()) {
            if (!_buffer->occupied(i))
                packet_ids.push_back(i + _buffer->abs_tail());
            i = (i + _buffer->psize()) % _buffer->rounded_cap();
        } 
    }

    if (packet_ids.size() > 0) {
        try {
            sockaddr_in my_addr = {};
            my_addr.sin_family      = AF_INET;
            my_addr.sin_addr.s_addr = INADDR_ANY;
            my_addr.sin_port        = 0;

            RexmitRequest request(my_addr, packet_ids);
            std::string request_str = request.to_str();
            log_info("[%s] sending rexmit request: %s", name.c_str(), request_str.c_str());
            // not checking the return value here, as if something went wront, the station will be switched soon
            // (aside from the assumption that the packet fits)
            if (request_str.length() == _ctrl_socket.sendto(request_str.c_str(), request_str.length(), (*_current_station)->ctrl_addr))
                log_error("[%s] sending rexmit request failed");
        } catch (const std::exception& e) {
            fatal("[%s] malformed rexmit request: %s", name.c_str());
        }
    }
}

void RexmitSenderWorker::run() {
    steady_clock::time_point prev_sleep = steady_clock::now();
    while (running) {
        std::this_thread::sleep_until(prev_sleep + _rtime);
        prev_sleep = steady_clock::now();
        order_retransmission();
    }
    log_debug("[%s] going down", name.c_str());
}
