#include "station_remover.hh"

#include <thread>

#define REMOVAL_THRESHOLD_SECS 20
#define REMOVAL_FREQ_MILLIS    500 //TODO: good enough?

using namespace std::chrono;

StationRemoverWorker::StationRemoverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    std::optional<std::string> _prio_station_name
) 
    : Worker(running)
    , _stations(stations)
    , _current_station(current_station) 
    , _prio_station_name(_prio_station_name)
    {}

void StationRemoverWorker::run() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(REMOVAL_FREQ_MILLIS));
        auto stations_lock        = _stations.lock();
        auto current_station_lock = _current_station.lock();
        for (auto it = _stations->begin(); it != _stations->end(); ) {
            if (steady_clock::now() - it->last_reply > seconds(REMOVAL_THRESHOLD_SECS)) {
                if (it == *_current_station)
                    reset_current_station();
                it = _stations->erase(it);
            } else
                ++it;
        }
    }
}

// this should be called under a mutex lock
void StationRemoverWorker::reset_current_station() {
    if (_prio_station_name) {
        // look for the preferred station
        for (auto it = _stations->begin(); it != _stations->end(); ++it) {
            if (it->name == _prio_station_name) {
                *_current_station = it;
                return;
            }
        }
    }

    // no preference or no success finding it
    if (!_stations->empty())
        *_current_station = _stations->begin();
    else
        *_current_station = _stations->end();
}
