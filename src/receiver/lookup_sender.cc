#include "lookup_sender.hh"

#include "../common/net.hh"
#include "../common/datagram.hh"

#include <thread>
#include <chrono>

#define LOOKUP_TIMEOUT_MILLIS 5000

LookupSenderWorker::LookupSenderWorker(
    const volatile sig_atomic_t& running,
    const sockaddr_in& discover_addr
)
    : Worker(running)
    , _discover_addr(discover_addr)
{
    _socket.set_broadcast();
    _socket.connect(discover_addr);
}

void LookupSenderWorker::run() {
    std::optional<std::chrono::steady_clock::time_point> prev_lookup_time;
    while (running) {
        sleep_until(prev_lookup_time, std::chrono::milliseconds(LOOKUP_TIMEOUT_MILLIS));
        _socket.sendto(LOOKUP_REQUEST_PREFIX, sizeof(LOOKUP_REQUEST_PREFIX) - 1, _discover_addr);
    }
}
