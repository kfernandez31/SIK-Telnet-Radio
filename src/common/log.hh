#pragma once

#include <cstddef>
#include <cerrno>
#include <cstdlib>
#include <cstring>

enum log_level_t {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
};

struct LogMsg {
    struct tm* time;
    log_level_t level;
    size_t line;
    const char* file;
    size_t length;
    char contents[];
};

#define log_trace(...) log_impl(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_impl(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_impl(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_impl(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_impl(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_impl(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)
#define fatal(...)                               \
do {                                             \
    log_fatal(__VA_ARGS__);                      \
    if (errno)                                   \
        log_fatal("Errno: %s", strerror(errno)); \
    exit(errno? errno : 1);                      \
} while (0)

#define log_impl(level, file, line, ...)                         \
do {                                                             \
    LogMsg* _msg = new_log_msg(level, file, line, __VA_ARGS__);  \
    print_log_msg(_msg);                                         \
} while(0)

LogMsg* new_log_msg(const log_level_t level, const char * const file, const size_t line, const char * const fmt, ...);
void free_log_msg(LogMsg* msg);
void print_log_msg(LogMsg* msg);
void logger_init(const bool log_to_file = false);
void logger_destroy();
