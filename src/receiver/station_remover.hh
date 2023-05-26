#pragma once

#include "../common/event_pipe.hh"
#include "../common/worker.hh"
#include "../common/synced_ptr.hh"
#include "../common/radio_station.hh"

#include <optional>
#include <string>

struct StationRemoverWorker : public Worker {
private:
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventPipe> _current_event;
    std::optional<std::string> _prio_station_name;

    void reset_current_station();
public:
    StationRemoverWorker() = delete;
    StationRemoverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventPipe>& current_event,
        std::optional<std::string> prio_station_name
    );
    
    void run() override;
};
