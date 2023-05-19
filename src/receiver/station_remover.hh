#pragma once

#include "../common/worker.hh"
#include "../common/ptr.hh"
#include "../common/radio_station.hh"

#include <optional>
#include <string>

struct StationRemoverWorker : public Worker {
private:
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    std::optional<std::string> _prio_station_name;

    void reset_current_station();
public:
    StationRemoverWorker() = delete;
    StationRemoverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        std::optional<std::string> _prio_station_name
    );
    
    void run() override;
};
