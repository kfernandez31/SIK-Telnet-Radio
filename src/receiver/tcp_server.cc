#include "tcp_server.hh"

#include "ui_menu.hh"
#include "../common/err.hh"
#include "../common/except.hh"

#define MY_EVENT    0
#define NETWORK     1
#define NUM_POLLFDS 2

TcpServerWorker::TcpServerWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<TcpClientSocketSet>& client_sockets,
    const SyncedPtr<EventQueue>& my_event,
    const std::shared_ptr<std::vector<pollfd>>& client_poll_fds,
    const std::shared_ptr<UiMenuWorker>& ui_menu,
    const int ui_port
)   : Worker(running)
    , _client_sockets(client_sockets) 
    , _my_event(my_event)
    , _client_poll_fds(client_poll_fds)
    , _ui_menu(ui_menu)
    , _socket(ui_port)
    {}

void TcpServerWorker::run() {
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd = _my_event->in_fd();
    poll_fds[NETWORK].fd  = _socket.fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    _socket.listen();
    while (running) {
        if (-1 == poll(poll_fds, NUM_POLLFDS, -1))
            fatal("poll");

        if (poll_fds[MY_EVENT].revents & POLLIN) {
            poll_fds[MY_EVENT].revents = 0;
            EventQueue::EventType event_val;
            {
                auto lock  = _my_event.lock();
                event_val  = _my_event->pop();
            }
            switch (event_val) {
                case EventQueue::EventType::TERMINATE:
                    assert(!running);
                default: break;
            }
        }

        if (poll_fds[NETWORK].revents & POLLIN) {
            poll_fds[NETWORK].revents = 0;
            try {
                int client_fd = _socket.accept();
                try_register_client(client_fd);
            } catch (const std::exception& e) {
                log_error(e.what());
            }
        }
    }
}

void TcpServerWorker::try_register_client(const int client_fd) {
    auto lock = _client_sockets.lock();
    for (size_t i = 0; i < MAX_CLIENTS; ++i) {
        if (_client_sockets->at(i).get() != nullptr) {
            assert(_client_poll_fds->at(i).fd != -1);
            _client_poll_fds->at(i).fd    = client_fd;
            _client_sockets->at(i) = std::make_unique<TcpClientSocket>(client_fd);
            UiMenuWorker::config_telnet_client(*_client_sockets->at(i));
            _ui_menu->greet_telnet_client(*_client_sockets->at(i));
            return;
        }
    }
    throw RadioException("Too many clients");
}
