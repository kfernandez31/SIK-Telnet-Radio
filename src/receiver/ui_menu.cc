#include "ui_menu.hh"

#include "../common/except.hh"

#include <thread>

#define MY_EVENT      0
#define SERVER        1
#define MIN_CLIENT_ID 2
#define REFRESH_RATE  30

using namespace std::chrono;

static const char* HORIZONTAL_BAR        = "------------------------------------------------------------------------";
static const char* PROGRAM_NAME          = "SIK Radio";
static const char* CHOSEN_STATION_PREFIX = " > ";

static const milliseconds REFRESH_TIMEOUT = milliseconds(1000 / REFRESH_RATE); 

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
    _command_map[ui::commands::UP]   = [&] { cmd_move_up();   };
    _command_map[ui::commands::DOWN] = [&] { cmd_move_down(); };

    _poll_fds = std::make_unique<pollfd[]>(TOTAL_POLLFDS);
    for (client_id_t i = 0; i < TOTAL_POLLFDS; ++i) {
        _poll_fds[i].fd      = -1;
        _poll_fds[i].events  = POLLIN | POLLOUT | POLLERR;
        _poll_fds[i].revents = 0;
    }
    _poll_fds[MY_EVENT].fd = _my_event->in_fd(); 
    _poll_fds[SERVER].fd   = _server_socket.fd(); 
}

void UiMenuWorker::config_client(const client_id_t id) {
    using namespace ui::telnet;
    std::ostringstream oss;
    oss
        << commands::IAC << commands::WILL << options::ECHO 
        << commands::IAC << commands::DO   << options::ECHO 
        << commands::IAC << commands::DO   << options::LINEMODE;
    _clients[id].socket.write(oss.str());
}

void UiMenuWorker::send_msg(const client_id_t id, const std::string& msg) {
    std::ostringstream oss;
    oss
        << ui::display::CLEAR << msg;
    try {
        _clients[id].socket.write(oss.str());
    } catch (const std::exception& e) {
        log_error("[%s] could not send message to client #%d : %s. Disconnecting", name.c_str(), id, e.what());
        disconnect_client(id);
    }
}

void UiMenuWorker::send_to_all(const std::string& msg) {
    for (client_id_t id = MIN_CLIENT_ID; id < UiMenuWorker::MAX_CLIENTS; ++id)
        if (_poll_fds[id].fd != -1)
            send_msg(id, msg);
}

void UiMenuWorker::greet_client(const client_id_t id) {
    send_msg(id, menu_to_str());
}

client_id_t UiMenuWorker::register_client(TcpClientSocket&& client_socket) {
    for (client_id_t i = MIN_CLIENT_ID; i < TOTAL_POLLFDS; ++i) {
        if (_poll_fds[i].fd == -1) {
            _poll_fds[i].fd      = client_socket.fd();
            _poll_fds[i].events  = POLLIN | POLLOUT | POLLERR;
            _poll_fds[i].revents = 0;
            _clients.emplace(i, std::move(client_socket));
            return i;
        }
    }
    throw RadioException("Could not register client");
}

void UiMenuWorker::accept_new_client() {
    try {
        TcpClientSocket client_socket = _server_socket.accept();
        log_info("[%s] accepted a new connection", name.c_str());
        client_id_t client_id = register_client(std::move(client_socket));
        log_info("[%s] successfully registered client #%d", name.c_str(), client_id);
        try {
            config_client(client_id);
            greet_client(client_id);
        } catch (const std::exception& e) {
            log_error("[%s] client #%d : could not initiate communication : %s. Disconnecting", name.c_str(), client_id, e.what());
            disconnect_client(client_id);
        }
    } catch (const std::exception &e) {
        log_error("[%s] error: %s", name.c_str(), e.what());
    }
}

void UiMenuWorker::handle_client_input(const client_id_t id) {
    try {
        char* cmd_ptr = _clients[id].cmd_buf + _clients[id].nread;
        bool eof = !_clients[id].socket.read(cmd_ptr, 1);
        _clients[id].nread++;
        if (eof) {
            log_info("[%s] client #%d disconnected", name.c_str(), id);
            disconnect_client(id);
        } else {
            bool success      = true;
            bool cmd_complete = false;
            switch (*cmd_ptr) {
                case ui::commands::Key::ESCAPE:
                    success = _clients[id].nread == 1;
                    break;
                case ui::commands::Key::DELIM:
                    success = _clients[id].nread == 2;
                    break;
                case ui::commands::Key::ARROW_UP:
                case ui::commands::Key::ARROW_DOWN: // intentional fall-through
                    cmd_complete = success = _clients[id].nread == 3;
                    break;
                default:
                    success = false;
            }

            if (!success) {
                log_error("[%s] client #%d : unrecognized command", name.c_str(), id);
                reset_client_input(id);
            } else if (cmd_complete)
                apply_cmd(id);
        }
    } catch (const std::exception& e) {
        log_error("[%s] client #%d : could not read data. Disconnecting", name.c_str(), id);
        disconnect_client(id);
    }
}

void UiMenuWorker::run() {
    log_info("[%s] listening on port %d", name.c_str(), _server_socket.port());
    _server_socket.listen();
    steady_clock::time_point last_sleep = steady_clock::now();
    while (running) {        
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
                    send_to_all(menu_to_str());
                default: break;
            }
        }

        if (_poll_fds[SERVER].revents & POLLIN) {
            _poll_fds[SERVER].revents = 0;
            accept_new_client();
        }

        std::this_thread::sleep_until(last_sleep + REFRESH_TIMEOUT);
        last_sleep = steady_clock::now();

        for (client_id_t id = MIN_CLIENT_ID; id < TOTAL_POLLFDS; ++id) {
            if (_poll_fds[id].fd != -1) {
                if (_poll_fds[id].revents & POLLERR) {
                    log_error("[%s] client #%d : poll error. Disconnecting", name.c_str(), id);
                    disconnect_client(id);
                    continue;
                }

                if (_poll_fds[id].revents & POLLIN)
                    handle_client_input(id);

                if (_poll_fds[id].revents & POLLOUT)
                    send_msg(id, menu_to_str());

                _poll_fds[id].revents = 0;     
            }
        }
    }
    log_debug("[%s] going down", name.c_str());
}

void UiMenuWorker::reset_client_input(const client_id_t id) {
    memset(_clients[id].cmd_buf, 0, sizeof(_clients[id].cmd_buf)); 
    _clients[id].nread = 0;
}

void UiMenuWorker::disconnect_client(const client_id_t id) {
    _clients.erase(id);
    _poll_fds[id].fd      = -1;
    _poll_fds[id].revents = 0;
    _poll_fds[id].events  = POLLIN | POLLOUT | POLLERR;
}

std::string UiMenuWorker::menu_to_str() {
    std::stringstream ss;
    ss
        << HORIZONTAL_BAR << ui::telnet::newline
        << PROGRAM_NAME   << ui::telnet::newline
        << HORIZONTAL_BAR << ui::telnet::newline;
    for (auto it = _stations->begin(); it != _stations->end(); ++it) {
        if (it == *_current_station)
            ss << BLINK(GREEN(CHOSEN_STATION_PREFIX)) << BOLD(it->name); 
        else 
            ss << it->name;
        ss << ui::telnet::newline;
    }
    ss << HORIZONTAL_BAR;
    return ss.str();
}

void UiMenuWorker::apply_cmd(const client_id_t id) {
    auto cmd_fun = _command_map.find(_clients[id].cmd_buf);
    assert(cmd_fun != _command_map.end());
    cmd_fun->second();
    reset_client_input(id);
}

void UiMenuWorker::cmd_move_up() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();

    if (!_stations->empty()) {
        if (*_current_station != _stations->begin()) {
            --*_current_station;
            auto event_lock = _audio_receiver_event.lock();
            _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
        }
    }
    send_to_all(menu_to_str());
}

void UiMenuWorker::cmd_move_down() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (!_stations->empty()) {
        if (*_current_station != std::prev(_stations->end())) {
            ++*_current_station;
            auto event_lock = _audio_receiver_event.lock();
            _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
        }
    }
    send_to_all(menu_to_str());
}
