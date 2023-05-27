#pragma once

#include "../common/synced_ptr.hh"
#include "../common/worker.hh"

struct EventPipe {
private:
    int _fds[2];
public:
    EventPipe();
    ~EventPipe();
 
    enum class EventType {
        NONE,
        STATION_CHANGE,
        PACKET_LOSS,
        SIG_INT,
    };

    int in_fd() const;
    void set_event(const EventType event_type);
    EventType get_event() const;
};
