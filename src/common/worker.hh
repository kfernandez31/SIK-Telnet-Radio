#pragma once

#include <csignal>

#include <optional>
#include <chrono>

static inline void send_msg(const int fd) {
    char msg_buf[1] = {42}; // contents not important
    if (-1 == write(fd, msg_buf, sizeof(msg_buf)))
        fatal("write");
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
