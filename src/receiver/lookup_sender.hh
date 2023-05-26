#pragma once

#include "../common/worker.hh"
#include "../common/udp_socket.hh"

#include <string>

struct LookupSenderWorker : public Worker {
private:
    UdpSocket _socket;
    sockaddr_in _discover_addr;
public:
    LookupSenderWorker() = delete;
    LookupSenderWorker(
        const volatile sig_atomic_t& running,
        const sockaddr_in& discover_addr
    );
    void run() override;
};
