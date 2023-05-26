#include "controller.hh"

#include "../common/err.hh"
#include "../common/net.hh"
#include "../common/datagram.hh"

ControllerWorker::ControllerWorker(
    const volatile sig_atomic_t& running, 
    const sockaddr_in& data_addr,
    const in_port_t ctrl_port,
    const std::string& station_name,
    const std::shared_ptr<RetransmitterWorker> retransmitter
)
    : Worker(running)
    , _station_name(station_name)
    , _retransmitter(retransmitter)
{
    _ctrl_socket.bind(ctrl_port);
}

void ControllerWorker::handle_lookup(const sockaddr_in& receiver_addr) {
    try {
        LookupReply reply(inet_ntoa(_data_socket.conn_addr.sin_addr), _data_port, _station_name);
        std::string reply_str = reply.to_str(); 
        _ctrl_socket.sendto(reply_str.c_str(), reply_str.size(), receiver_addr);
    } catch (const std::exception& e) {
        logerr("Malformed lookup reply: %s", e.what());
    }
}

void ControllerWorker::run() {
    char req_buf[MTU + 1] = {0};
    while (running) {
        memset(req_buf, 0, sizeof(req_buf));
        sockaddr_in src_addr;
        _ctrl_socket.recvfrom(req_buf, sizeof(req_buf), src_addr);
        if (0 == strncmp(req_buf, LOOKUP_REQUEST_PREFIX, sizeof(LOOKUP_REQUEST_PREFIX)))
            handle_lookup(src_addr);
        else {
            try {
                _retransmitter->emplace_job(req_buf);
            } catch (const std::exception& e) {
                logerr("Malformed rexmit request: %s", e.what());
            }
        }
    }
}
