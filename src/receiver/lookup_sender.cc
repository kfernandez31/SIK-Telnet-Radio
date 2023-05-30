#include "lookup_sender.hh"

#include "../common/net.hh"
#include "../common/datagram.hh"

#include <poll.h>

#include <thread>
#include <chrono>

#define MY_EVENT    0
#define NETWORK     1
#define NUM_POLLFDS 2

using namespace std::chrono;

static const milliseconds LOOKUP_TIMEOUT = milliseconds(5000);

LookupSenderWorker::LookupSenderWorker(
    const volatile sig_atomic_t& running,
    const SyncedPtr<EventQueue>& my_event,
    const std::shared_ptr<UdpSocket>& ctrl_socket,
    const sockaddr_in& discover_addr
)
    : Worker(running, "LookupSender")
    , _my_event(my_event)
    , _ctrl_socket(ctrl_socket)
    , _discover_addr(discover_addr)
    {}


void LookupSenderWorker::run() {
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd      = _my_event->in_fd();
    poll_fds[MY_EVENT].events  = POLLIN;
    poll_fds[NETWORK].fd       = _ctrl_socket->fd();
    poll_fds[NETWORK].events   = POLLOUT;
    for (size_t i = 0; i < NUM_POLLFDS; ++i)
        poll_fds[i].revents = 0;
    
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
                default: break;
            }
        }

        if (poll_fds[NETWORK].revents & POLLOUT) {
            poll_fds[NETWORK].revents = 0;
            std::this_thread::sleep_until(prev_sleep + LOOKUP_TIMEOUT);
            prev_sleep = steady_clock::now();
            log_info("[%s] sending request...", name.c_str());
            try {
                LookupRequest req;
                std::string req_str = req.to_str();
                _ctrl_socket->sendto(req_str.c_str(), req_str.size(), _discover_addr);
            } catch (std::exception& e) {
                log_error("[%s] malformed lookup request : %s", name.c_str(), e.what());
            }
        }
    }
}
