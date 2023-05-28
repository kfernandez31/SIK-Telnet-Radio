#pragma once

#include "../common/event_queue.hh"
#include "../common/worker.hh"
#include "../common/udp_socket.hh"
#include "../common/radio_station.hh"
#include "../common/synced_ptr.hh"

#include <string>

struct LookupReceiverWorker : public Worker {
private:
    UdpSocket _ctrl_socket;
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventQueue> _my_event;
    SyncedPtr<EventQueue> _audio_receiver_event;
    SyncedPtr<EventQueue> _ui_menu_event;
    std::optional<std::string> _prio_station_name;
public:
    LookupReceiverWorker() = delete;
    LookupReceiverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventQueue>& my_event,  
        const SyncedPtr<EventQueue>& audio_receiver_event,
        const SyncedPtr<EventQueue>& ui_menu_event,
        const std::optional<std::string> prio_station_name,
        const in_port_t ctrl_port
    );
    
    void run() override;
};
