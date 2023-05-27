#include "tcp_client_handler.hh"

#include "../common/err.hh"
#include "../common/except.hh"
#include "ui.hh"

#include <cassert>

static const char* HORIZONTAL_BAR        = "------------------------------------------------------------------------";
static const char* PROGRAM_NAME          = "SIK Radio";
static const char* CHOSEN_STATION_PREFIX = " > ";

TcpClientHandlerWorker::TcpClientHandlerWorker(
    const volatile sig_atomic_t& running,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventPipe>& my_event,
    const SyncedPtr<EventPipe>& audio_recvr_event,
    const SyncedPtr<TcpClientSocketSet>& client_sockets,
    const std::shared_ptr<std::vector<pollfd>>& poll_fds
)
    : Worker(running) 
    , _stations(stations)
    , _current_station(current_station)
    , _my_event(my_event)
    , _audio_recvr_event(audio_recvr_event)
    , _client_sockets(client_sockets)
    , _poll_fds(poll_fds)
{
    _command_map.emplace(ui::keys::ARROW_UP,   [&] { cmd_move_up();   });
    _command_map.emplace(ui::keys::ARROW_DOWN, [&] { cmd_move_down(); });
}


void TcpClientHandlerWorker::run() {
    const size_t NUM_CLIENTS = _client_sockets->size();
    const size_t INTERNAL_EVENT = 1 + NUM_CLIENTS;

    while (running) {
        if (-1 == poll(_poll_fds->data(), 1 + NUM_CLIENTS, -1))
            fatal("poll");
        
        if (_poll_fds->at(INTERNAL_EVENT).revents & POLLIN) {
            _poll_fds->at(INTERNAL_EVENT).revents = 0;
            EventPipe::EventType event_val;
            {
                auto lock  = _my_event.lock();
                event_val  = _my_event->get_event();
                _my_event->set_event(EventPipe::EventType::NONE);
            }
            switch (event_val) {
                case EventPipe::EventType::SIG_INT:
                    assert(!running);
                    return;
                case EventPipe::EventType::STATION_CHANGE:
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

std::string TcpClientHandlerWorker::menu_to_str() {
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

void TcpClientHandlerWorker::send_msg(TcpClientSocket& client, const std::string& msg) {
    client.out() 
        << ui::telnet::options::NAOFFD << ui::display::CLEAR 
        << msg 
        << TcpClientSocket::OutStream::flush;
}

void TcpClientHandlerWorker::send_to_all(const std::string& msg) {
    auto lock = _client_sockets.lock();
    for (auto &sock : *_client_sockets)
        if (sock.get() != nullptr)
        send_msg(*sock, msg);
}

void TcpClientHandlerWorker::config_telnet_client(TcpClientSocket& client) {
    using namespace ui::telnet;
    client.out() 
        << commands::IAC << commands::WILL << options::ECHO 
        << commands::IAC << commands::DO   << options::ECHO 
        << commands::IAC << commands::DO   << options::LINEMODE 
        << TcpClientSocket::OutStream::flush;
}

void TcpClientHandlerWorker::greet_telnet_client(TcpClientSocket& client) {
    TcpClientHandlerWorker::send_msg(client, menu_to_str());
}

std::string TcpClientHandlerWorker::read_cmd(TcpClientSocket& socket) {
    if (socket.in().eof())
        throw RadioException("Client disconnected");
    std::string cmd_buf;
    socket.in().getline(cmd_buf);
    return cmd_buf;
}

void TcpClientHandlerWorker::apply_cmd(std::string& cmd_buf) {
    auto cmd_fun = _command_map.find(cmd_buf);
    if (cmd_fun == _command_map.end())
        throw RadioException("Unknown command");
    cmd_fun->second();
}

void TcpClientHandlerWorker::cmd_move_up() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (*_current_station != _stations->begin()) {
        --*_current_station;
        auto event_lock = _audio_recvr_event.lock();
        _audio_recvr_event->set_event(EventPipe::EventType::STATION_CHANGE);
    }
    send_to_all(menu_to_str());
}

void TcpClientHandlerWorker::cmd_move_down() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (*_current_station != std::prev(_stations->end())) {
        ++*_current_station;
        auto event_lock = _audio_recvr_event.lock();
        _audio_recvr_event->set_event(EventPipe::EventType::STATION_CHANGE);
    }
    send_to_all(menu_to_str());
}
