#pragma once

#include "../common/event_queue.hh"
#include "../common/datagram.hh"
#include "../common/worker.hh"
#include "../common/udp_socket.hh"
#include "../common/radio_station.hh"
#include "../common/synced_ptr.hh"

#include <string>
#include <memory>

struct LookupReceiverWorker : public Worker {
private:
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventQueue> _my_event;
    SyncedPtr<EventQueue> _audio_receiver_event;
    SyncedPtr<EventQueue> _ui_menu_event;
    std::shared_ptr<UdpSocket> _ctrl_socket;
    std::optional<std::string> _prio_station_name;
    void handle_lookup_reply(LookupReply& reply, const sockaddr_in& src_addr);
public:
    LookupReceiverWorker() = delete;
    LookupReceiverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventQueue>& my_event,  
        const SyncedPtr<EventQueue>& audio_receiver_event,
        const SyncedPtr<EventQueue>& ui_menu_event,
        const std::shared_ptr<UdpSocket>& ctrl_socket,
        const std::optional<std::string> prio_station_name
    );
    
    void run() override;
};
