#pragma once

#include "../common/worker.hh"
#include "../common/radio_station.hh"
#include "../common/err.hh"
#include "../common/tcp_socket.hh"
#include "../common/ptr.hh"

#include <netinet/in.h>

#include <string>
#include <optional>

// TODO: maintain a set of clients, poll them

struct UiMenuWorker : public Worker {
private:
    TcpSocket _socket;
    std::optional<std::string> _prio_station_name;
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    int _audio_receiver_fd;

    enum class cmd_t {
        MOVE_UP,
        MOVE_DOWN,
    };

    void display_menu();
    void apply_command(const cmd_t cmd);
    void report_station_change();
public:
    UiMenuWorker() = delete;
    UiMenuWorker(
        const volatile sig_atomic_t& running, 
        const in_port_t port, 
        const std::optional<std::string> prio_station_name,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const int audio_receiver_fd
    );
    ~UiMenuWorker();

    void run() override;
};
