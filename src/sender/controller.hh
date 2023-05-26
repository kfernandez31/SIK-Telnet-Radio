#pragma once

#include "retransmitter.hh"

#include "../common/worker.hh"
#include "../common/udp_socket.hh"

#include "netinet/in.h"

#include <memory>
#include <string>

struct ControllerWorker : public Worker {
private:
    UdpSocket _data_socket, _ctrl_socket;
    in_port_t _data_port;
    std::string _station_name;
    std::shared_ptr<RetransmitterWorker> _retransmitter;

    void handle_lookup(const sockaddr_in& receiver_addr);
public:
    ControllerWorker() = delete;
    ControllerWorker(
        const volatile sig_atomic_t& running, 
        const sockaddr_in& data_addr,
        const in_port_t ctrl_port,
        const std::string& station_name,
        const std::shared_ptr<RetransmitterWorker> retransmitter
    );
    
    void run() override;
};
