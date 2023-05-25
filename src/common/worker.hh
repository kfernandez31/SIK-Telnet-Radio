#pragma once

#include <unistd.h>
#include <csignal>

#include <optional>
#include <chrono>

#include "err.hh"

static inline void send_msg(const int fd) {
    char msg_buf[1] = {42}; // contents not important
    if (-1 == write(fd, msg_buf, sizeof(msg_buf)))
        fatal("write");
}

static inline void order_worker_termination(int* fd) {
    if (-1 != *fd) {
        send_msg(*fd);
        if (-1 == close(*fd))
            fatal("close");
        *fd = -1;
    }
}

class Worker {
protected:
    const volatile sig_atomic_t& running;
    void sleep_until(std::optional<std::chrono::steady_clock::time_point>& prev_time, std::chrono::milliseconds timeout);
public:
    Worker(const volatile sig_atomic_t& running);
    virtual ~Worker() {}
    virtual void run() = 0; 
};
