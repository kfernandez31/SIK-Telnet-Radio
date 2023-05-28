#pragma once

#include <csignal>

class Worker {
protected:
    const volatile sig_atomic_t& running;
public:
    Worker(const volatile sig_atomic_t& running) : running(running) {};
    virtual ~Worker() {}
    virtual void run() = 0; 
};
