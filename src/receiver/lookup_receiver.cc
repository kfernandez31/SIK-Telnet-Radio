#include "lookup_receiver.hh"

#include "../common/net.hh"
#include "../common/radio_station.hh"

#include <poll.h>

using namespace std::chrono;

#define MY_EVENT    0
#define NETWORK     1
#define NUM_POLLFDS 2

static const seconds RECEIVING_TIMEOUT = seconds(10);

LookupReceiverWorker::LookupReceiverWorker(
    const volatile sig_atomic_t& running,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventQueue>& my_event,
    const SyncedPtr<EventQueue>& audio_receiver_event,
    const SyncedPtr<EventQueue>& ui_menu_event,
    const std::shared_ptr<UdpSocket>& ctrl_socket,
    const std::optional<std::string> prio_station_name
)
    : Worker(running, "LookupReceiver")
    , _stations(stations)
    , _current_station(current_station)
    , _my_event(my_event)
    , _audio_receiver_event(audio_receiver_event)
    , _ui_menu_event(ui_menu_event)
    , _ctrl_socket(ctrl_socket)
    , _prio_station_name(prio_station_name)
    {
        _ctrl_socket->set_receiving_timeout(RECEIVING_TIMEOUT.count());
    }

void LookupReceiverWorker::handle_lookup_reply(LookupReply& reply, const sockaddr_in& src_addr) {
    RadioStation station(src_addr, reply);
    log_info("[%s] got lookup reply : [mcast_addr = %s, data_port = %hu, name = %s]", name.c_str(), reply.mcast_addr.c_str(), reply.data_port, reply.name.c_str());
    auto stations_lock = _stations.lock();
    auto it = _stations->find(station);
    if (it != _stations->end()) {
        log_info("[%s] station %s is still alive", name.c_str(), station.name.c_str());
        it->update_last_reply();
    } else {
        log_info("[%s] new station: %s", name.c_str(), station.name.c_str());
        auto inserted_it = _stations->insert(station).first;
        if (_stations->size() == 1 || _prio_station_name == station.name) {
            auto current_station_lock = _current_station.lock();
            (*_current_station) = inserted_it;
            auto lock = _audio_receiver_event.lock();
            _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
        }
        auto lock = _ui_menu_event.lock();
        _ui_menu_event->push(EventQueue::EventType::STATION_ADDED);
    }
}

void LookupReceiverWorker::run() {
    char reply_buf[UDP_MAX_DATA_SIZE + 1] = {0};
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd = _my_event->in_fd();
    poll_fds[NETWORK].fd  = _ctrl_socket->fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    while (running) {
        if (poll(poll_fds, NUM_POLLFDS, -1)) == -1)
            fatal("poll");

        if (poll_fds[MY_EVENT].revents & POLLIN) {
            poll_fds[MY_EVENT].revents = 0;
            EventQueue::EventType event_val = _my_event->pop();
            switch (event_val) {
                case EventQueue::EventType::TERMINATE:
                    return;
                default: break;
            }
        }

        if (poll_fds[NETWORK].revents & POLLIN) {
            poll_fds[NETWORK].revents = 0;
            sockaddr_in src_addr;
            memset(reply_buf, 0, sizeof(reply_buf));
            if (sizeof(reply_buf) - 1 != _ctrl_socket->recvfrom(reply_buf, sizeof(reply_buf) - 1, src_addr))
                log_warn("[%s] waited too long for lookup reply", name.c_str());
            log_info("[%s] got lookup reply: %s", name.c_str(), reply_buf);
            try {
                LookupReply reply(reply_buf);
                handle_lookup_reply(reply, src_addr);
            } catch (const std::exception& e) {
                log_error("[%s] malformed lookup reply: %s", name.c_str(), e.what());
            }
        }
    }
    log_debug("[%s] going down", name.c_str());
}
