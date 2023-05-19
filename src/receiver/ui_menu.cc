#include "ui_menu.hh"

#include <unistd.h>

#include <chrono>

static const char* CMD_UP   = "\e[A";
static const char* CMD_DOWN = "\e[B";

static const char* ANSI_CLEAR = "\x1b[2J";

static const char* HORIZONTAL_BAR        = "------------------------------------------------------------------------";
static const char* PROGRAM_NAME          = "SIK Radio";
static const char* CHOSEN_STATION_PREFIX = " > ";

UiMenuWorker::UiMenuWorker(
    const volatile sig_atomic_t& running, 
    const in_port_t port, 
    const std::optional<std::string> prio_station_name,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const int audio_receiver_fd
)
    : Worker(running)
    , _socket(port, 42, 42) //TODO: change these to some sensible values
    , _prio_station_name(prio_station_name)
    , _stations(stations)
    , _current_station(current_station)
    , _audio_receiver_fd(audio_receiver_fd)
    {}

UiMenuWorker::~UiMenuWorker() {
    if (-1 == close(_audio_receiver_fd))
        fatal("close");
}

void UiMenuWorker::apply_command(const cmd_t cmd) {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (cmd == cmd_t::MOVE_UP && *_current_station != _stations->begin()) {
        --*_current_station;
        report_station_change();
    }
    else if (cmd == cmd_t::MOVE_DOWN && *_current_station != std::prev(_stations->end())) {
        ++*_current_station;
        report_station_change();
    }
}

void UiMenuWorker::report_station_change() {
    send_msg(_audio_receiver_fd);
}

void UiMenuWorker::run() {
    while (running) {
        std::string cmd_buf;
        try {
            _socket.in().getline(cmd_buf);
        } catch (std::exception& e) {
            logerr(e.what());
        }

        if (cmd_buf == CMD_UP)
            apply_command(cmd_t::MOVE_UP);
        else if (cmd_buf == CMD_DOWN)
            apply_command(cmd_t::MOVE_DOWN);
        else
            logerr("Unknown command");   
        
        display_menu();
    }
}

void UiMenuWorker::display_menu() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    _socket.out()
        // << ANSI_CLEAR //TODO: maybe add this?
        << HORIZONTAL_BAR << '\n'
        << PROGRAM_NAME   << '\n'
        << HORIZONTAL_BAR << '\n';

    for (auto it = _stations->begin(); it != _stations->end(); ++it) {
        if (it == *_current_station)
            _socket.out() << CHOSEN_STATION_PREFIX;
        _socket.out() << it->name << '\n';
    }

    _socket.out() << HORIZONTAL_BAR << _socket.out().endl;
}
