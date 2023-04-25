#pragma once

#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <cstring>

// Evaluate `x`: if non-zero, describe it as a standard error code and exit with an error.
#define CHECK(x)                                                          \
    do {                                                                  \
        int err = (x);                                                    \
        if (err != 0) {                                                   \
            fprintf(stderr, "Error: %s returned %d in %s at %s:%d\n%s\n", \
                #x, err, __func__, __FILE__, __LINE__, strerror(err));    \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

// Evaluate `x`: if negative, print an error message and exit with an error.
#define VERIFY(x)                                                         \
    do {                                                                  \
        int err = (x);                                                    \
        if (err < 0) {                                                    \
           PRINT_ERRNO();                                                 \
        }                                                                 \
    } while (0)

// Evaluate `x`: if false, print an error message and exit with an error.
#define ENSURE(x)                                                         \
    do {                                                                  \
        bool result = (x);                                                \
        if (!result) {                                                    \
            fprintf(stderr, "Error: %s was false in %s at %s:%d\n",       \
                #x, __func__, __FILE__, __LINE__);                        \
            exit(EXIT_FAILURE);                                           \
        }                                                                 \
    } while (0)

// Check if errno is non-zero, and if so, print an error message and exit with an error.
#define PRINT_ERRNO()                                                  \
    do {                                                               \
        if (errno != 0) {                                              \
            fprintf(stderr, "Error: errno %d in %s at %s:%d\n%s\n",    \
              errno, __func__, __FILE__, __LINE__, strerror(errno));   \
            exit(EXIT_FAILURE);                                        \
        }                                                              \
    } while (0)

// Set `errno` to 0 and evaluate `x`. If `errno` changed, describe it and exit.
#define CHECK_ERRNO(x)                                                             \
    do {                                                                           \
        errno = 0;                                                                 \
        (void) (x);                                                                \
        PRINT_ERRNO();                                                             \
    } while (0)

// Print an error message and exit with an error.
void fatal(const char *fmt, ...);

// Logs a message to stderr
void eprintln(const char* fmt, ...);
