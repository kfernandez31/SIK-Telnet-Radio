#include "lookup_receiver.hh"

#include "../common/err.hh"
#include "../common/net.hh"
#include "../common/datagram.hh"
#include "../common/radio_station.hh"

LookupReceiverWorker::LookupReceiverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventPipe>& current_event,
    const std::optional<std::string> prio_station_name,
    const in_port_t ctrl_port
) 
    : Worker(running)
    , _stations(stations)
    , _current_station(current_station) 
    , _current_event(current_event)
    , _prio_station_name(prio_station_name)
    {}

void LookupReceiverWorker::run() {
    char reply_buf[LOOKUP_REPLY_MAX_LEN + 1] = {0};

    while (running) {
        sockaddr_in sender_addr;
        _ctrl_socket.recvfrom(reply_buf, sizeof(reply_buf), sender_addr);
        try {
            LookupReply reply(reply_buf);
            RadioStation station(sender_addr, reply.mcast_addr, reply.data_port, reply.name);

            auto stations_lock = _stations.lock();
            auto it = _stations->find(station);
            if (it == _stations->end()) {
                logerr("New station: %s", station.name.c_str());
            } else
                _stations->erase(it);
                
            logerr("Got lookup reply from station: %s", station.name.c_str());
            auto inserted_it = _stations->insert(station).first;

            if (_stations->size() == 1 || _prio_station_name == station.name) {
                auto current_station_lock = _current_station.lock();
                (*_current_station) = inserted_it;
                _current_event->put_event(EventPipe::EventType::STATION_CHANGE);
            }
        } catch (const std::exception& e) {
            logerr("Malformed lookup reply: %s", e.what());
        }
    }
}
