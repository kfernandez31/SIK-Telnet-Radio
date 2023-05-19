#include "err.hh"

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

[[noreturn]] void fatal_impl(const char* file, const size_t line, const char* msg) {
    fprintf(stderr, "Error (%s:%zu): %s.", file, line, msg);
    if (errno)
        fprintf(stderr, "\nErrno: %s.", strerror(errno));
    fprintf(stderr, "\n");
    exit(errno ? errno : EXIT_FAILURE);
}

[[noreturn]] void vfatal_impl(const char* file, const size_t line, const char* msg, ...) {
    fprintf(stderr, "Error (%s:%zu): ", file, line);

    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    if (errno)
        fprintf(stderr, ".\nErrno: %s.\n", strerror(errno));
    else 
        fprintf(stderr, ".\n");

    exit(errno ? errno : EXIT_FAILURE);
}

void logerr(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}
