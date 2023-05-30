#pragma once

#include "../common/worker.hh"
#include "../common/synced_ptr.hh"
#include "../common/event_queue.hh"
#include "../common/udp_socket.hh"

#include <string>
#include <memory>

struct LookupSenderWorker : public Worker {
private:
    SyncedPtr<EventQueue> _my_event;
    std::shared_ptr<UdpSocket> _ctrl_socket;
    sockaddr_in _discover_addr;
public:
    LookupSenderWorker() = delete;
    LookupSenderWorker(
        const volatile sig_atomic_t& running,
        const SyncedPtr<EventQueue>& my_event,
        const std::shared_ptr<UdpSocket>& ctrl_socket,
        const sockaddr_in& discover_addr
    );
    void run() override;
};
