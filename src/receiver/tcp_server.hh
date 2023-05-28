#pragma once

#include "ui_menu.hh"
#include "../common/event_queue.hh"
#include "../common/worker.hh"
#include "../common/tcp_socket.hh"
#include "../common/synced_ptr.hh"

#include "poll.h"

#include <cstddef>

#include <memory>
#include <vector>


struct TcpServerWorker : public Worker {
private:
    SyncedPtr<TcpClientSocketSet> _client_sockets;
    SyncedPtr<EventQueue> _my_event;
    std::shared_ptr<std::vector<pollfd>> _client_poll_fds;
    std::shared_ptr<UiMenuWorker> _ui_menu;
    TcpServerSocket _socket;

    void try_register_client(const int client_fd);
public:
    TcpServerWorker() = delete;
    TcpServerWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<TcpClientSocketSet>& client_sockets,
        const SyncedPtr<EventQueue>& my_event,
        const std::shared_ptr<std::vector<pollfd>>& client_poll_fds,
        const std::shared_ptr<UiMenuWorker>& ui_menu,
        const int ui_port
    );

    void run() override;
    static const size_t MAX_CLIENTS = 42;
};
