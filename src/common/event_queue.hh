#pragma once

#include "../common/synced_ptr.hh"
#include "../common/worker.hh"

struct EventQueue {
private:
    int _fds[2];
public:
    EventQueue();
    ~EventQueue();
 
    enum class EventType {
        CURRENT_STATION_CHANGED,
        STATION_ADDED,
        STATION_REMOVED,
        CLIENT_ADDED,
        NEW_JOBS,
        TERMINATE,
    };

    int in_fd() const;
    void push(const EventType event_type);
    EventType pop() const;
};
