#pragma once

#include "log.hh"

#include <string>
#include <csignal>

/**
 * @class Worker
 * @brief Abstract base class representing a background worker.
 *
 * The `Worker` class defines a common interface for worker threads or processes
 * that perform tasks while their `running` flag is set.
 *
 * The worker monitors the `running` flag, which is typically modified by a signal
 * handler (e.g., SIGTERM) to allow for graceful termination.
 */
class Worker {
protected:
    const volatile sig_atomic_t& running; ///< Flag indicating whether the worker should continue running.
    const std::string name;               ///< Worker name, used for logging.
public:
    /**
     * @brief Constructs a Worker instance.
     * @param running A reference to a volatile `sig_atomic_t` flag controlling execution state.
     * @param name The name of the worker, primarily used for logging.
     */
    Worker(const volatile sig_atomic_t& running, const std::string& name) : running(running), name(name) {}

    /**
     * @brief Virtual destructor that logs termination.
     */
    virtual ~Worker() {
        log_warn("[%s] terminating...", name.c_str());
    }

    /**
     * @brief Pure virtual method that must be implemented by derived classes.
     *
     * This method contains the worker's main execution logic.
     * Derived classes should periodically check `running` to handle graceful shutdowns.
     */
    virtual void run() = 0;
};