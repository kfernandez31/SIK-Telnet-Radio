#include "lookup_receiver.hh"

#include "../common/err.hh"
#include "../common/net.hh"
#include "../common/datagram.hh"
#include "../common/radio_station.hh"

#include <poll.h>

#define MY_EVENT    0
#define NETWORK     1
#define NUM_POLLFDS 2

LookupReceiverWorker::LookupReceiverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventQueue>& my_event,  
    const SyncedPtr<EventQueue>& audio_receiver_event,
    const SyncedPtr<EventQueue>& ui_menu_event,
    const std::optional<std::string> prio_station_name,
    const in_port_t ctrl_port
) 
    : Worker(running)
    , _stations(stations)
    , _current_station(current_station) 
    , _my_event(my_event)
    , _audio_receiver_event(audio_receiver_event)
    , _ui_menu_event(ui_menu_event)
    , _prio_station_name(prio_station_name)
{
    _ctrl_socket.bind(ctrl_port);
}

void LookupReceiverWorker::run() {
    char reply_buf[MTU + 1] = {0};
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd = _my_event->in_fd();
    poll_fds[NETWORK].fd  = _ctrl_socket.fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

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
                    return;
                default: break;
            }
        }

        // got a new response
        if (poll_fds[NETWORK].revents & POLLIN) {
            poll_fds[NETWORK].revents = 0;
            sockaddr_in sender_addr;
            try {
                _ctrl_socket.recvfrom(reply_buf, sizeof(reply_buf), sender_addr);
                LookupReply reply(reply_buf);
                RadioStation station(sender_addr, reply.mcast_addr, reply.data_port, reply.name);

                auto stations_lock = _stations.lock();
                auto it = _stations->find(station);
                if (it != _stations->end()) {
                    logerr("Station %s is still alive", station.name.c_str());
                    it->update_last_reply();
                } else {
                    logerr("New station: %s", station.name.c_str());
                    if (_stations->size() == 0 || _prio_station_name == station.name) {
                        auto current_station_lock = _current_station.lock();
                        (*_current_station) = _stations->insert(station).first;
                        auto lock = _audio_receiver_event.lock();
                        _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
                    }
                    auto lock = _ui_menu_event.lock();
                    _ui_menu_event->push(EventQueue::EventType::STATION_ADDED);
                }
            } catch (const std::exception& e) {
                logerr("Malformed lookup reply: %s", e.what());
            }
        }
    }
}
