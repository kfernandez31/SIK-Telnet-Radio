#include "controller.hh"

#include "../common/net.hh"
#include "../common/datagram.hh"

#include <poll.h>

#define MY_EVENT    0
#define NETWORK     1
#define NUM_POLLFDS 2

ControllerWorker::ControllerWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<EventQueue>& my_event,
    const SyncedPtr<EventQueue>& retransmitter_event,
    const SyncedPtr<std::queue<RexmitRequest>>& rexmit_job_queue,
    const std::string& mcast_addr,
    const in_port_t data_port,
    const std::string& name,
    const in_port_t ctrl_port
)
    : Worker(running, "Controller")
    , _my_event(my_event)
    , _retransmitter_event(retransmitter_event)
    ,  _rexmit_job_queue(rexmit_job_queue)
    , _lookup_reply(mcast_addr, data_port, name)
{
    _ctrl_socket.bind(ctrl_port);
}


void ControllerWorker::handle_lookup_request(const sockaddr_in& src_addr, [[maybe_unused]] LookupRequest&& req) {
    auto reply = _lookup_reply.to_str();
    if (reply.size() != _ctrl_socket.sendto(reply.c_str(), reply.size(), src_addr))
        log_error("[%s] unable to send lookup reply", name.c_str());
}

void ControllerWorker::handle_rexmit_request(const sockaddr_in& src_addr, RexmitRequest&& req) {
    auto jobs_lock = _rexmit_job_queue.lock();
    _rexmit_job_queue->push(std::move(req));
    auto event_lock = _retransmitter_event.lock();
    _retransmitter_event->push(EventQueue::EventType::NEW_JOBS);
}

void ControllerWorker::run() {
    char req_buf[UDP_MAX_DATA_SIZE + 1] = {0};
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd      = _my_event->in_fd();
    poll_fds[NETWORK].fd       = _ctrl_socket.fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].revents = 0;
        poll_fds[i].events  = POLLIN;
    }

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

        if (poll_fds[NETWORK].revents & POLLIN) {
            poll_fds[NETWORK].revents = 0;
            sockaddr_in src_addr;
            DatagramType req_type = DatagramType::None;
            req_buf[_ctrl_socket.recvfrom(req_buf, sizeof(req_buf) - 1, src_addr)] = '\0';

            try {
                LookupRequest req(req_buf);
                req_type = DatagramType::LookupRequest;
                log_info("[%s] got lookup request", name.c_str());
                handle_lookup_request(src_addr, std::move(req));
            } catch (...) {}
            try {
                RexmitRequest req(src_addr, req_buf);
                req_type = DatagramType::RexmitRequest;
                log_info("[%s] got rexmit request", name.c_str());
                handle_rexmit_request(src_addr, std::move(req));
            } catch (...) {}

            if (req_type == DatagramType::None)
                log_error("[%s] unrecognized request", name.c_str());
        }
    }
}
