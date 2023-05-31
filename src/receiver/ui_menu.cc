#include "ui_menu.hh"

#include "ui.hh"

#define MY_EVENT          0
#define SERVER            1
#define MIN_CLIENT_POLLFD 2

static const char* HORIZONTAL_BAR        = "------------------------------------------------------------------------";
static const char* PROGRAM_NAME          = "SIK Radio";
static const char* CHOSEN_STATION_PREFIX = " > ";

UiMenuWorker::UiMenuWorker(
    const volatile sig_atomic_t& running,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventQueue>& my_event,
    const SyncedPtr<EventQueue>& audio_receiver_event,
    const in_port_t ui_port
)
    : Worker(running, "UiMenu") 
    , _stations(stations)
    , _current_station(current_station)
    , _my_event(my_event)
    , _audio_receiver_event(audio_receiver_event)
    , _server_socket(ui_port)
{
    _command_map[ui::keys::ARROW_UP]   = [&] { cmd_move_up();   };
    _command_map[ui::keys::ARROW_DOWN] = [&] { cmd_move_down(); };

    _poll_fds = std::make_unique<pollfd[]>(TOTAL_POLLFDS);
    for (client_id_t i = 0; i < TOTAL_POLLFDS; ++i) {
        _poll_fds[i].fd      = -1;
        _poll_fds[i].events  = POLLIN;
        _poll_fds[i].revents = 0;
    }
    _poll_fds[MY_EVENT].fd    = _my_event->in_fd(); 
    _poll_fds[SERVER].fd = _server_socket.fd(); 
}

size_t UiMenuWorker::active_clients() const {
    return _client_sockets.size();
}

void UiMenuWorker::config_client(const client_id_t id) {
    using namespace ui::telnet;
    _client_sockets[id]->out() 
        << commands::IAC << commands::WILL << options::ECHO 
        << commands::IAC << commands::DO   << options::ECHO 
        << commands::IAC << commands::DO   << options::LINEMODE 
        << TcpClientSocket::OutStream::flush;
}

void UiMenuWorker::send_msg(const client_id_t id, const std::string& msg) {
    _client_sockets[id]->out()
        << ui::telnet::options::NAOFFD << ui::display::CLEAR 
        << msg 
        << TcpClientSocket::OutStream::flush;
}

void UiMenuWorker::send_to_all(const std::string& msg) {
    for (client_id_t i = MIN_CLIENT_POLLFD; i < UiMenuWorker::MAX_CLIENTS; ++i)
        if (_poll_fds[i].fd != -1)
            send_msg(i, msg);
}

void UiMenuWorker::greet_client(const client_id_t id) {
    send_msg(id, menu_to_str());
}

client_id_t UiMenuWorker::try_register_client(const int client_fd) {
    for (client_id_t i = MIN_CLIENT_POLLFD; i < TOTAL_POLLFDS; ++i) {
        if (_poll_fds[i].fd == -1) {
            _poll_fds[i].fd      = client_fd;
            _poll_fds[i].events  = POLLIN;
            _poll_fds[i].revents = 0;
            _client_sockets[i]   = std::make_unique<TcpClientSocket>(client_fd);
            return i;
        }
    }
    return -1;
}

#include <thread>
#include <chrono>

void UiMenuWorker::run() {
    log_info("[%s] listening on port %d", name.c_str(), _server_socket.port());
    _server_socket.listen();
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        
        if (-1 == poll(_poll_fds.get(), TOTAL_POLLFDS, -1))
            fatal("poll");

        if (_poll_fds[MY_EVENT].revents & POLLIN) {
            _poll_fds[MY_EVENT].revents = 0;
            EventQueue::EventType event_val = _my_event->pop();
            switch (event_val) {
                case EventQueue::EventType::TERMINATE:
                    return;
                case EventQueue::EventType::STATION_ADDED:
                case EventQueue::EventType::STATION_REMOVED:
                case EventQueue::EventType::CURRENT_STATION_CHANGED: // intentional fall-through
                    log_info("[%s] updating menu...", name.c_str());
                    send_to_all(menu_to_str());
                default: break;
            }
        }

        if (_poll_fds[SERVER].revents & POLLIN) {
            _poll_fds[SERVER].revents = 0;
            int client_fd = _server_socket.accept();
            if (client_fd == -1) {
                log_error("[%s] accepting a new connection failed", name.c_str());
                continue;
            }

            log_info("[%s] accepted a new connection", name.c_str());
            client_id_t id = try_register_client(client_fd);

            if (id == -1) {
                log_error("[%s] could not register client #%zu : too many clients", name.c_str(), active_clients());
                continue;
            }
            
            log_info("[%s] successfully registered client #%zu", name.c_str(), active_clients());
            config_client(id);
            greet_client(id);
        }

        for (client_id_t i = MIN_CLIENT_POLLFD; i < TOTAL_POLLFDS; ++i) {
            client_id_t client_id = i - MIN_CLIENT_POLLFD;

            if (_poll_fds[i].fd != -1 && _poll_fds[i].revents & POLLIN) {
                _poll_fds[i].revents = 0;
                log_debug("[%s] got CLIENT = %d with FD = %d", name.c_str(), client_id, _client_sockets[i]->fd());
                
                if (_client_sockets[i]->in().eof()) {
                    log_info("[%s] client #%zu disconnected", name.c_str(), client_id);
                    disconnect_client(i);
                } else {
                    try {
                        std::string cmd_buf = read_cmd(i);
                        log_debug("[%s] got new command from client #%zu: %s", name.c_str(), client_id, cmd_buf.c_str());
                        if (!apply_cmd(cmd_buf))
                            log_error("[%s] client #%zu error : unknown command", name.c_str(), client_id);
                    } catch (std::exception& e) {
                        log_error("[%s] client %zu error: %s. disconnecting...", name.c_str(), client_id, e.what());
                        disconnect_client(i);
                    }
                }
            }
        }
    }
}

void UiMenuWorker::disconnect_client(const client_id_t id) {
    _client_sockets.erase(id);
    _poll_fds[id].fd      = -1;
    _poll_fds[id].revents = 0;
    _poll_fds[id].events  = POLLIN;
}

std::string UiMenuWorker::menu_to_str() {
    std::stringstream ss;
    ss
        << HORIZONTAL_BAR << ui::telnet::newline
        << PROGRAM_NAME   << ui::telnet::newline
        << HORIZONTAL_BAR << ui::telnet::newline;
    for (auto it = _stations->begin(); it != _stations->end(); ++it) {
        if (it == *_current_station)
            ss << HIGHLIGHT(CHOSEN_STATION_PREFIX);
        ss << it->name << ui::telnet::newline;
    }
    ss << HORIZONTAL_BAR;
    return ss.str();
}

std::string UiMenuWorker::read_cmd(const client_id_t id) {
    std::string cmd_buf;
    _client_sockets[id]->in().getline(cmd_buf);
    return cmd_buf;
}

bool UiMenuWorker::apply_cmd(std::string& cmd_buf) {
    auto cmd_fun = _command_map.find(cmd_buf);
    if (cmd_fun == _command_map.end())
        return false;
    cmd_fun->second();
    return true;
}

void UiMenuWorker::cmd_move_up() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (*_current_station != _stations->begin()) {
        log_debug("[%s] moving menu UP", name.c_str()); //TODO: wywal
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
        log_debug("[%s] moving menu DOWN", name.c_str());   //TODO: wywal
        ++*_current_station;
        auto event_lock = _audio_receiver_event.lock();
        _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
    }
    send_to_all(menu_to_str());
}
