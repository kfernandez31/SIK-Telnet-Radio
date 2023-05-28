#include "lookup_sender.hh"

#include "../common/net.hh"
#include "../common/datagram.hh"

#include <thread>
#include <chrono>

using namespace std::chrono;

static milliseconds LOOKUP_TIMEOUT = milliseconds(5000);

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
    steady_clock::time_point prev_sleep = steady_clock::now();
    while (running) {
        std::this_thread::sleep_until(prev_sleep + LOOKUP_TIMEOUT);
        prev_sleep = steady_clock::now();
        _socket.sendto(LOOKUP_REQUEST_PREFIX, sizeof(LOOKUP_REQUEST_PREFIX) - 1, _discover_addr);
    }
}
