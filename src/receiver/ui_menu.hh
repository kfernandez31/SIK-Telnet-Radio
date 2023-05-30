#pragma once

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

struct UiMenuWorker : public Worker {
private:
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventQueue> _my_event;
    SyncedPtr<EventQueue> _audio_receiver_event;
    SyncedPtr<TcpClientSocketSet> _client_sockets;
    std::shared_ptr<std::vector<pollfd>> _poll_fds;
    std::map<std::string, std::function<void()>> _command_map;

    void cmd_move_up();
    void cmd_move_down();

    std::string read_cmd(TcpClientSocket& socket);
    void        apply_cmd(std::string& cmd_buf);

    void send_to_all(const std::string& msg);
    void disconnect_client(const size_t client_id);
public:
    UiMenuWorker() = delete;
    UiMenuWorker(
        const volatile sig_atomic_t& running,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventQueue>& my_event,
        const SyncedPtr<EventQueue>& audio_receiver_event,
        const SyncedPtr<TcpClientSocketSet>& client_sockets,
        const std::shared_ptr<std::vector<pollfd>>& poll_fds
    );

    void run() override;

    std::string menu_to_str();
    void greet_telnet_client(TcpClientSocket& client_socket);

    static void config_telnet_client(TcpClientSocket& client_socket);
    static void send_msg(TcpClientSocket& client_socket, const std::string& msg);
};
