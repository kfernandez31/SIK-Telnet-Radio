#include "client_manager.hh"

#include "../common/err.hh"

ClientManager::run() {
    bool ok = false;
    while (running) {
    _socket.listen();

    if (_clients_count < MAX_CLIENTS) {
        try {
            _socket.accept();
            lock_guard<mutex> manager_lk(tcp_manager->cv_m);
            shared_ptr<TCPclientHandler> t_sh = SmartThread::create<TCPclientHandler>
                (tcp_manager, _socket.msg_sock_descriptor());
            tcp_manager->clients_handlers.push_back(t_sh);
            tcp_manager->clients_count++;
            t_sh->start();
        } catch (const std::exception& e) {
            logerr(e.what());
        }
    }
}

// poll na
//   - socketcie TCP -> 
//     --> dopisz nowego do listy
//   - menu klienta
//    --> apply command

// muszę mieć pollfds
// lista musi być pod mutexem (i musi być occupied[])
// getter na fd dla socketa
// 
