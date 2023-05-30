#pragma once

#include "log.hh"

#include <string>

#include <csignal>

class Worker {
protected:
    const volatile sig_atomic_t& running;
    const std::string name;
public:
    Worker(const volatile sig_atomic_t& running, const std::string& name) : running(running), name(name) {};
    virtual ~Worker() {
        log_info("[%s] terminating...", name.c_str());
    }
    virtual void run() = 0;
};
