#pragma once

#include "../common/event_queue.hh"
#include "../common/worker.hh"
#include "../common/synced_ptr.hh"
#include "../common/radio_station.hh"

#include <optional>
#include <string>

struct StationRemoverWorker : public Worker {
private:
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventQueue> _audio_receiver_event;
    SyncedPtr<EventQueue> _ui_menu_event;
    std::optional<std::string> _prio_station_name;

    void reset_current_station();
public:
    StationRemoverWorker() = delete;
    StationRemoverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventQueue>& audio_receiver_event,
        const SyncedPtr<EventQueue>& ui_menu_event,
        const std::optional<std::string> prio_station_name
    );
    
    void run() override;
};
