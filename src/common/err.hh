#pragma once

#include "log.hh"

#include <cstddef>
#include <cerrno>
#include <cstdlib>
// #define  fatal(msg)  fatal_impl(__FILE__, __LINE__, msg)

#define fatal(...)          \
do {                        \
    log_fatal(__VA_ARGS__); \
    exit(errno? errno : 1); \
} while (0)

[[noreturn]] void fatal_impl(const char* file, const size_t line, const char* msg);
void logerr(const char* fmt, ...);
