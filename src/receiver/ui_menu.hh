#pragma once

#include "ui.hh"
#include "../common/worker.hh"
#include "../common/tcp_socket.hh"
#include "../common/synced_ptr.hh"
#include "../common/event_queue.hh"
#include "../common/radio_station.hh"

#include "poll.h"

#include <cstddef>

#include <string>
#include <map>
#include <memory>
#include <functional>

struct TcpClient {
    TcpClientSocket socket;
    TcpClient() : socket(-1) {}
    TcpClient(TcpClientSocket&& socket_) : socket(-1) { this->socket = std::move(socket_); }
    size_t nread = 0;
    char cmd_buf[ui::commands::MAX_CMD_LEN + 1] = {0};
};

using client_id_t = int;

struct UiMenuWorker : public Worker {
private:
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventQueue> _my_event;
    SyncedPtr<EventQueue> _audio_receiver_event;

    std::unique_ptr<pollfd[]> _poll_fds;
    TcpServerSocket _server_socket;
    std::map<int, TcpClient> _clients;

    std::map<std::string, std::function<void()>> _command_map;

    void cmd_move_up();
    void cmd_move_down();

    void accept_new_client();
    client_id_t register_client(TcpClientSocket&& client_socket);
    void config_client(const client_id_t id);
    void greet_client(const client_id_t id);
    void handle_client_input(const client_id_t id);
    void apply_cmd(const client_id_t id);
    void reset_client_input(const client_id_t id);
    void send_msg(const client_id_t id, const std::string& msg);
    void send_to_all(const std::string& msg);
    void disconnect_client(const client_id_t id);
public:
    UiMenuWorker() = delete;
    UiMenuWorker(
        const volatile sig_atomic_t& running,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventQueue>& my_event,
        const SyncedPtr<EventQueue>& audio_receiver_event,
        const in_port_t ui_port
    );

    void run() override;

    std::string menu_to_str();

    static inline const size_t MAX_CLIENTS   = 42;
    static inline const size_t TOTAL_POLLFDS = 2 + MAX_CLIENTS;
};
