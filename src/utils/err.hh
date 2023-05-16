#pragma once

#include <cstddef>

#define  fatal(msg)  fatal_impl(__FILE__, __LINE__, msg)
#define vfatal(...) vfatal_impl(__FILE__, __LINE__, __VA_ARGS__)

[[noreturn]] void fatal_impl(const char* file, const size_t line, const char* msg);
[[noreturn]] void vfatal_impl(const char* file, const size_t line, const char* msg, ...);
void logerr(const char* fmt, ...);
