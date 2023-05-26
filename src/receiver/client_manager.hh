#pragma once

#include "../common/worker.hh"
#include "../common/tcp_socket.hh"

#include <cstddef>

struct ClientManager : public Worker {
private:
    TcpSocket _socket;
    size_t _clients_count;
public:
    static const size_t MAX_CLIENTS = 42;
    void run() override;
};
