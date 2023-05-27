#pragma once

#include "../common/worker.hh"
#include "../common/tcp_socket.hh"
#include "../common/synced_ptr.hh"
#include "../common/event_pipe.hh"
#include "../common/radio_station.hh"

#include "poll.h"

#include <cstddef>

#include <string>
#include <map>
#include <memory>
#include <functional>

struct TcpClientHandlerWorker : public Worker {
private:
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventPipe> _my_event;
    SyncedPtr<EventPipe> _audio_recvr_event;
    SyncedPtr<TcpClientSocketSet> _client_sockets;
    std::shared_ptr<std::vector<pollfd>> _poll_fds;
    std::map<std::string, std::function<void()>> _command_map;

    void cmd_move_up();
    void cmd_move_down();

    std::string menu_to_str();
    void        send_to_all(const std::string& msg);
    
    std::string read_cmd(TcpClientSocket& socket);
    void        apply_cmd(std::string& cmd_buf);
public:
    TcpClientHandlerWorker() = delete;
    TcpClientHandlerWorker(
        const volatile sig_atomic_t& running,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventPipe>& my_event,
        const SyncedPtr<EventPipe>& audio_recvr_event,
        const SyncedPtr<TcpClientSocketSet>& client_sockets,
        const std::shared_ptr<std::vector<pollfd>>& poll_fds
    );

    void run() override;
};
