#include "worker.hh"

#include "err.hh"

#include <thread>

Worker::Worker(const volatile sig_atomic_t& running) : running(running) {}

void Worker::sleep_until(std::optional<std::chrono::steady_clock::time_point>& prev_time, std::chrono::milliseconds timeout) {
    if (prev_time) {
        auto delta_time = std::chrono::steady_clock::now() - *prev_time;
        if (delta_time < timeout) {
            try {
                std::this_thread::sleep_for(timeout - delta_time);
            } catch (std::exception& e) {
                logerr("sleep interrupted");
            }
        }
    }
    prev_time = std::chrono::steady_clock::now();
}
