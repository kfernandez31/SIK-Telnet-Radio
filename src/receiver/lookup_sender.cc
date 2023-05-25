#include "lookup_sender.hh"

#include "../common/net.hh"
#include "../common/datagram.hh"

#include <thread>
#include <chrono>

#define LOOKUP_TIMEOUT_MILLIS 5000

LookupSenderWorker::LookupSenderWorker(
    const volatile sig_atomic_t& running,
    const in_port_t ctrl_port, const 
    std::string& discover_addr_str
): Worker(running) {
    //TODO:
    //socket.set_reuseaddr();
    //socket.bind_local_port(port);
    sockaddr_in discover_addr = get_addr(discover_addr_str.c_str(), ctrl_port);
    //socket.enable_mcast_send(discover_addr);
}

void LookupSenderWorker::run() {
    std::optional<std::chrono::steady_clock::time_point> prev_lookup_time;
    while (running) {
        sleep_until(prev_lookup_time, std::chrono::milliseconds(LOOKUP_TIMEOUT_MILLIS));
        _socket.write(LOOKUP_REQUEST_PREFIX, sizeof(LOOKUP_REQUEST_PREFIX) - 1);
    }
}
