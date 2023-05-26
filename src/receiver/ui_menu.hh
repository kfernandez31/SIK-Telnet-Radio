#pragma once

#include "../common/event_pipe.hh"
#include "../common/worker.hh"
#include "../common/radio_station.hh"
#include "../common/err.hh"
#include "../common/tcp_socket.hh"
#include "../common/synced_ptr.hh"

#include <netinet/in.h>

#include <string>
#include <optional>

// TODO: maintain a set of clients, poll them

struct UiMenuWorker : public Worker {
private:
    TcpSocket _socket;
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventPipe> _current_event;
    std::optional<std::string> _prio_station_name;

    enum class Command {
        MOVE_UP,
        MOVE_DOWN,
    };

    void display_menu();
    void apply_command(const Command cmd);
    void report_station_change();
public:
    UiMenuWorker() = delete;
    UiMenuWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventPipe>& current_event,
        const in_port_t port, 
        const std::optional<std::string> prio_station_name
    );

    void run() override;
};
