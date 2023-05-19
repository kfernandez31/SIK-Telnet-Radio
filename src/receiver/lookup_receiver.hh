#pragma once

#include "../common/worker.hh"
#include "../common/udp_socket.hh"
#include "../common/radio_station.hh"
#include "../common/ptr.hh"

#include <string>

struct LookupReceiverWorker : public Worker {
private:
    UdpSocket socket;
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    std::optional<std::string> _prio_station_name;
public:
    LookupReceiverWorker() = delete;
    LookupReceiverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const std::optional<std::string> prio_station_name
    );
    void run() override;
};
