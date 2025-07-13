#pragma once

#include "../common/synced_ptr.hh"
#include "../common/worker.hh"

#include <unistd.h>

/**
 * @class EventQueue
 * @brief Implements a lightweight inter-thread or inter-process event queue.
 *
 * This class wraps a simple **pipe-based event queue** to facilitate **event-driven communication**.
 * Events can be pushed into the queue and later popped in a FIFO manner.
 */
struct EventQueue {
private:
    int _fds[2]; ///< File descriptors for the read (0) and write (1) ends of the pipe.

public:
    /**
     * @brief Constructs an `EventQueue` and initializes a pipe.
     * @throws Calls `fatal()` if the pipe cannot be created.
     */
    EventQueue();

    /**
     * @brief Destructor that closes both ends of the pipe.
     * @throws Calls `fatal()` if closing the pipe fails.
     */
    ~EventQueue();

    /**
     * @enum EventType
     * @brief Represents different types of events in the queue.
     */
    enum class EventType {
        CURRENT_STATION_CHANGED, ///< Indicates a change in the current station.
        STATION_ADDED,           ///< A new station has been added.
        STATION_REMOVED,         ///< A station has been removed.
        CLIENT_ADDED,            ///< A new client has connected.
        NEW_JOBS,                ///< Indicates that new jobs have been queued.
        TERMINATE,               ///< Signals termination of the event loop.
    };

    /**
     * @brief Retrieves the file descriptor for reading events.
     * @return The read-end file descriptor of the queue.
     */
    int in_fd() const;

    /**
     * @brief Pushes an event into the queue.
     * @param event_type The event type to push.
     * @throws Calls `fatal()` if writing to the queue fails.
     */
    void push(EventType event_type);

    /**
     * @brief Pops an event from the queue.
     * @return The event type that was retrieved.
     * @throws Calls `fatal()` if reading from the queue fails.
     */
    EventType pop() const;
};