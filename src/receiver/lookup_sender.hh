#pragma once

#include "../common/worker.hh"
#include "../common/udp_socket.hh"

#include <string>

struct LookupSenderWorker : public Worker {
private:
    UdpSocket _socket;
public:
    LookupSenderWorker() = delete;
    LookupSenderWorker(
        const volatile sig_atomic_t& running,
        const in_port_t ctrl_port, 
        const std::string& discover_addr_str
    );
    void run() override;
};
