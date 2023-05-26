#pragma once

#include <unistd.h>
#include <csignal>

#include <optional>
#include <chrono>

#include "err.hh"

class Worker {
protected:
    const volatile sig_atomic_t& running;
    void sleep_until(std::optional<std::chrono::steady_clock::time_point>& prev_time, std::chrono::milliseconds timeout);
public:
    Worker(const volatile sig_atomic_t& running);
    virtual ~Worker() {}
    virtual void run() = 0; 
};
