#include "./mem.hh"
#include "./err.hh"

#include <cstring>

uint8_t* my_memcpy(uint8_t* buffer, const void* src, const size_t nbytes) {
    memcpy(buffer, src, nbytes);
    return buffer + nbytes;
}