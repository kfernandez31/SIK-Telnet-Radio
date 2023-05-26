#include "station_remover.hh"

#include <thread>
#include <algorithm>

#define REMOVAL_THRESHOLD_SECS 20
#define REMOVAL_FREQ_MILLIS    500 //TODO: good enough?

using namespace std::chrono;

StationRemoverWorker::StationRemoverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventPipe>& current_event,
    std::optional<std::string> prio_station_name
) 
    : Worker(running)
    , _stations(stations)
    , _current_station(current_station) 
    , _current_event(current_event)
    , _prio_station_name(prio_station_name)
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
    StationSet::iterator new_station = _stations->end();

    if (_prio_station_name) {
        for (auto it = _stations->begin(); it != _stations->end(); ++it) {
            if (it->name == _prio_station_name) {
                new_station = it;
                break;
            }
        }
    }

    if (new_station != _stations->end()) {
        *_current_station = new_station;
    } else if (!_stations->empty())
        *_current_station = _stations->begin();
    else
        *_current_station = _stations->end();

    auto lock = _current_event.lock();
    _current_event->put_event(EventPipe::EventType::STATION_CHANGE);
}
