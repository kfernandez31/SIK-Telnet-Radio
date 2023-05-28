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
    const SyncedPtr<EventQueue>& audio_receiver_event,
    const SyncedPtr<EventQueue>& ui_menu_event,
    const std::optional<std::string> prio_station_name
) 
    : Worker(running)
    , _stations(stations)
    , _current_station(current_station) 
    , _audio_receiver_event(audio_receiver_event)
    , _ui_menu_event(ui_menu_event)
    , _prio_station_name(prio_station_name)
    {}

void StationRemoverWorker::run() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(REMOVAL_FREQ_MILLIS));
        auto stations_lock        = _stations.lock();
        auto current_station_lock = _current_station.lock();
        bool removed_any = 0;
        bool removed_current = false;
        for (auto it = _stations->begin(); it != _stations->end(); ) {
            if (steady_clock::now() - it->last_reply < seconds(REMOVAL_THRESHOLD_SECS))
                ++it;
            else {
                removed_any     |= true;
                removed_current |= (it == *_current_station);
                it = _stations->erase(it);
            } 
        }

        if (removed_any) {
            auto lock = _ui_menu_event.lock();
            _ui_menu_event->push(EventQueue::EventType::STATION_REMOVED);
            if (removed_current) 
                reset_current_station();
        }
    }
}

// this should be called under a mutex lock
void StationRemoverWorker::reset_current_station() {
    StationSet::iterator prio_station = _stations->end();

    if (_prio_station_name) {
        for (auto it = _stations->begin(); it != _stations->end(); ++it) {
            if (it->name == _prio_station_name) {
                prio_station = it;
                break;
            }
        }
    }

    if (prio_station != _stations->end()) {
        *_current_station = prio_station;
    } else if (!_stations->empty())
        *_current_station = _stations->begin();
    else
        *_current_station = _stations->end();

    auto lock = _audio_receiver_event.lock();
    _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
}
