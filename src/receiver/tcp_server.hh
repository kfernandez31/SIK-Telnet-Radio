#pragma once

#include "tcp_client_handler.hh"
#include "../common/worker.hh"
#include "../common/tcp_socket.hh"
#include "../common/synced_ptr.hh"

#include "poll.h"

#include <cstddef>

#include <memory>
#include <vector>

struct TcpServerWorker : public Worker {
private:
    TcpServerSocket _socket;
    SyncedPtr<TcpClientSocketSet> _client_sockets;
    std::shared_ptr<std::vector<pollfd>> _poll_fds;
    std::shared_ptr<TcpClientHandlerWorker> _client_handler;

    void try_register_client(const int client_fd);
public:
    TcpServerWorker() = delete;
    TcpServerWorker(
        const volatile sig_atomic_t& running, 
        const int ui_port,
        const SyncedPtr<TcpClientSocketSet>& client_sockets,
        const std::shared_ptr<std::vector<pollfd>>& poll_fds,
        const std::shared_ptr<TcpClientHandlerWorker>& client_handler
    );

    void run() override;
    static const size_t MAX_CLIENTS = 42;
};
