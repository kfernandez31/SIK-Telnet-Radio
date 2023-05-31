#include "station_remover.hh"

#include <thread>
#include <algorithm>

using namespace std::chrono;

static const seconds      REMOVAL_THRESHOLD(20);
static const milliseconds REMOVAL_FREQUENCY(500);

using namespace std::chrono;

StationRemoverWorker::StationRemoverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventQueue>& audio_receiver_event,
    const SyncedPtr<EventQueue>& ui_menu_event,
    const std::optional<std::string> prio_station_name
) 
    : Worker(running, "StationRemover")
    , _stations(stations)
    , _current_station(current_station) 
    , _audio_receiver_event(audio_receiver_event)
    , _ui_menu_event(ui_menu_event)
    , _prio_station_name(prio_station_name)
    {}

void StationRemoverWorker::remove_inactive() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    bool removed_any = 0;
    bool removed_current = false;
    for (auto it = _stations->begin(); it != _stations->end(); ) {
        if (steady_clock::now() - it->last_reply < REMOVAL_THRESHOLD)
            ++it;
        else {
            removed_any     |= true;
            removed_current |= (it == *_current_station);
            log_info("[%s] removing %s station: %s", name.c_str(), (it == *_current_station)? "(current)" : "", it->name.c_str());
            it = _stations->erase(it);
        } 
    }
    if (removed_any) {
        auto lock = _ui_menu_event.lock();
        _ui_menu_event->push(EventQueue::EventType::STATION_REMOVED);
        if (removed_current) {
            log_info("[%s] resetting current station", name.c_str());
            reset_current_station();
        }
    }
}

void StationRemoverWorker::run() {
    steady_clock::time_point prev_sleep = steady_clock::now();
    while (running) {
        std::this_thread::sleep_until(prev_sleep + REMOVAL_FREQUENCY);
        prev_sleep = steady_clock::now();
        remove_inactive();
    }
}

// this should be called under a mutex lock
void StationRemoverWorker::reset_current_station() {
    //TODO: wywal logi
    StationSet::iterator prio_station = _stations->end();

    if (_prio_station_name) {
        for (auto it = _stations->begin(); it != _stations->end(); ++it) {
            if (it->name == _prio_station_name) {
                prio_station = it;
                log_debug("[%s] found my favorite station!", name.c_str()); 
                break;
            }
        }
    }

    if (prio_station != _stations->end()) {
        *_current_station = prio_station;
    } else if (!_stations->empty()) {
        log_debug("[%s] resetted to first station", name.c_str());
        *_current_station = _stations->begin();
    } else {
        log_info("[%s] no stations left. Going silent...", name.c_str());
        *_current_station = _stations->end();
    }

    auto lock = _audio_receiver_event.lock();
    _audio_receiver_event->push(EventQueue::EventType::CURRENT_STATION_CHANGED);
}
