#include "./mem.hh"
#include "./err.hh"

#include <unistd.h>
#include <cstring>

uint8_t* my_memcpy(uint8_t* buffer, const void* src, const size_t nbytes) {
    memcpy(buffer, src, nbytes);
    return buffer + nbytes;
}

void my_write(const int fd, const void* src, const size_t nbytes) {
    ssize_t nwritten = write(fd, src, nbytes);
    if (-1 == nwritten || nwritten != (ssize_t)nbytes)
        fatal("write");
}
