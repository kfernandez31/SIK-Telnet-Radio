#include "ui_menu.hh"

#include "../common/err.hh"
#include "../common/except.hh"
#include "ui.hh"

#include <cassert>

static const char* HORIZONTAL_BAR        = "------------------------------------------------------------------------";
static const char* PROGRAM_NAME          = "SIK Radio";
static const char* CHOSEN_STATION_PREFIX = " > ";

UiMenuWorker::UiMenuWorker(
    const volatile sig_atomic_t& running,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventQueue>& my_event,
    const SyncedPtr<EventQueue>& audio_receiver_event,
    const SyncedPtr<TcpClientSocketSet>& client_sockets,
    const std::shared_ptr<std::vector<pollfd>>& poll_fds
)
    : Worker(running) 
    , _stations(stations)
    , _current_station(current_station)
    , _my_event(my_event)
    , _audio_receiver_event(audio_receiver_event)
    , _client_sockets(client_sockets)
    , _poll_fds(poll_fds)
{
    _command_map.emplace(ui::keys::ARROW_UP,   [&] { cmd_move_up();   });
    _command_map.emplace(ui::keys::ARROW_DOWN, [&] { cmd_move_down(); });
}


void UiMenuWorker::run() {
    const size_t NUM_CLIENTS = _client_sockets->size();
    const size_t MY_EVENT = 1 + NUM_CLIENTS;

    while (running) {
        if (-1 == poll(_poll_fds->data(), 1 + NUM_CLIENTS, -1))
            fatal("poll");
        
        if (_poll_fds->at(MY_EVENT).revents & POLLIN) {
            _poll_fds->at(MY_EVENT).revents = 0;
            EventQueue::EventType event_val;
            {
                auto lock  = _my_event.lock();
                event_val  = _my_event->pop();
            }
            switch (event_val) {
                case EventQueue::EventType::TERMINATE:
                    return;
                case EventQueue::EventType::STATION_ADDED:
                case EventQueue::EventType::STATION_REMOVED:
                case EventQueue::EventType::CURRENT_STATION_CHANGED: // intentional fall-through
                    send_to_all(menu_to_str());
                default: break;
            }
        }
        auto lock = _client_sockets.lock();
        for (size_t i = 1; i < NUM_CLIENTS; ++i) {
            if (_poll_fds->at(i).fd != -1 && _poll_fds->at(i).revents & POLLIN) {
                assert(_client_sockets->at(i).get() != nullptr);
                _poll_fds->at(i).revents = 0;
                if (_client_sockets->at(i)->in().eof()) {
                    _poll_fds->at(i).fd = -1;
                    _client_sockets->at(i)->~TcpClientSocket(); // free system resources
                } else {
                    try {
                        std::string cmd_buf = read_cmd(*_client_sockets->at(i));
                        apply_cmd(cmd_buf);
                    } catch (std::exception& e) {
                        logerr(e.what());
                    }
                }
            }
        }
    }
}

std::string UiMenuWorker::menu_to_str() {
    std::stringstream ss;
    ss
        << HORIZONTAL_BAR << '\n'
        << PROGRAM_NAME   << '\n'
        << HORIZONTAL_BAR << '\n';
    for (auto it = _stations->begin(); it != _stations->end(); ++it) {
        if (it == *_current_station)
            ss << HIGHLIGHT(CHOSEN_STATION_PREFIX);
        ss << it->name << '\n';
    }
    ss << HORIZONTAL_BAR;
    return ss.str();
}

void UiMenuWorker::send_msg(TcpClientSocket& client, const std::string& msg) {
    client.out() 
        << ui::telnet::options::NAOFFD << ui::display::CLEAR 
        << msg 
        << TcpClientSocket::OutStream::flush;
}

void UiMenuWorker::send_to_all(const std::string& msg) {
    auto lock = _client_sockets.lock();
    for (auto &sock : *_client_sockets)
        if (sock.get() != nullptr)
        send_msg(*sock, msg);
}

void UiMenuWorker::config_telnet_client(TcpClientSocket& client) {
    using namespace ui::telnet;
    client.out() 
        << commands::IAC << commands::WILL << options::ECHO 
        << commands::IAC << commands::DO   << options::ECHO 
        << commands::IAC << commands::DO   << options::LINEMODE 
        << TcpClientSocket::OutStream::flush;
}

void UiMenuWorker::greet_telnet_client(TcpClientSocket& client) {
    UiMenuWorker::send_msg(client, menu_to_str());
}

std::string UiMenuWorker::read_cmd(TcpClientSocket& socket) {
    if (socket.in().eof())
        throw RadioException("Client disconnected");
    std::string cmd_buf;
    socket.in().getline(cmd_buf);
    return cmd_buf;
}

void UiMenuWorker::apply_cmd(std::string& cmd_buf) {
    auto cmd_fun = _command_map.find(cmd_buf);
    if (cmd_fun == _command_map.end())
        throw RadioException("Unknown command");
    cmd_fun->second();
}

void UiMenuWorker::cmd_move_up() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (*_current_station != _stations->begin()) {
        --*_current_station;
        auto event_lock = _audio_receiver_event.lock();
        _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
    }
    send_to_all(menu_to_str());
}

void UiMenuWorker::cmd_move_down() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (*_current_station != std::prev(_stations->end())) {
        ++*_current_station;
        auto event_lock = _audio_receiver_event.lock();
        _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
    }
    send_to_all(menu_to_str());
}
