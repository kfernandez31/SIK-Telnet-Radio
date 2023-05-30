#pragma once

#include "retransmitter.hh"

#include "../common/event_queue.hh"
#include "../common/synced_ptr.hh"
#include "../common/worker.hh"
#include "../common/udp_socket.hh"

#include "netinet/in.h"

#include <memory>
#include <string>
#include <queue>

struct ControllerWorker : public Worker {
private:
    SyncedPtr<EventQueue> _my_event;
    SyncedPtr<EventQueue> _retransmitter_event;
    SyncedPtr<std::queue<RexmitRequest>> _rexmit_job_queue;
    UdpSocket _ctrl_socket;
    LookupReply _lookup_reply;

    void handle_lookup_request(const sockaddr_in& src_addr, [[maybe_unused]] const LookupRequest& req);
    void handle_rexmit_request(const sockaddr_in& src_addr, RexmitRequest&& req);
public:
    ControllerWorker() = delete;
    ControllerWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<EventQueue>& my_event,
        const SyncedPtr<EventQueue>& retransmitter_event,
        const SyncedPtr<std::queue<RexmitRequest>>& rexmit_job_queue,
        const std::string& mcast_addr,
        const in_port_t data_port,
        const std::string& name,
        const in_port_t ctrl_port
    );
    
    void run() override;
};
