#include "./buffer.hh"
#include "./err.hh"

#include <unistd.h>
#include <cstring>

uint8_t* bufcpy(uint8_t* buffer, const void* src, const size_t nbytes) {
    memcpy(buffer, src, nbytes);
    return buffer + nbytes;
}

size_t cyclical_write(const int fd, const uint8_t* buf, const size_t bsize, const size_t start, const size_t nbytes) {
    size_t end = (start + nbytes) % bsize;
    ssize_t nwritten;
    if (start <= end) {
        nwritten = write(fd, buf + start, nbytes);
        VERIFY(nwritten);
        ENSURE((size_t)nwritten == nbytes);
    } else {
        size_t first_chunk_size = bsize - start;
        nwritten = write(fd, buf + start, first_chunk_size);
        VERIFY(nwritten);
        ENSURE((size_t)nwritten == first_chunk_size);
        nwritten = write(fd, buf, end);
        VERIFY(nwritten);
        ENSURE((size_t)nwritten == nbytes);
    }

    return end;
}

size_t cyclical_memcpy(uint8_t* dest, const uint8_t* src, const size_t bsize, const size_t start, const size_t nbytes) {
    size_t end = (start + nbytes) % bsize;
    
    if (start <= end) {
        memcpy(dest + start, src, nbytes);
    } else {
        size_t first_chunk_size = bsize - start;
        memcpy(dest + start, src, first_chunk_size);
        memcpy(dest, src + first_chunk_size, end);
    }

    return end;
}

bool is_between(const size_t tail, const size_t head, const size_t bsize, const size_t idx) {
    if (tail <= head) {
        return tail <= idx && idx <= head;
    } else {
        return idx <= head || (tail <= idx && idx < bsize);
    }
}
