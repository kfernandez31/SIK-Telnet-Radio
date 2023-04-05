#include "buffer.h"

//#include <string.h>
#include <cstring>
#include <unistd.h>

uint8_t* bufcpy(uint8_t* buffer, const void* src, const size_t nbytes) {
    memcpy(buffer, src, nbytes);
    return buffer + nbytes;
}

// size_t cyclical_write(const uint8_t* buf, const size_t bsize, const size_t start, const size_t nbytes) {
//     size_t end = (start + nbytes) % bsize;
    
//     if (start <= end) {
//         write(STDOUT_FILENO, buf + start, nbytes);
//     } else {
//         size_t first_chunk_size = bsize - start;
//         write(STDOUT_FILENO, buf + start, first_chunk_size);
//         write(STDOUT_FILENO, buf, end);
//     }

//     return end;
// }

// size_t cyclical_memcpy(uint8_t* dest, const uint8_t* src, const size_t bsize, const size_t start, const size_t nbytes) {
//     size_t end = (start + nbytes) % bsize;
    
//     if (start <= end) {
//         memcpy(dest + start, src, nbytes);
//     } else {
//         size_t first_chunk_size = bsize - start;
//         memcpy(dest + start, src, first_chunk_size);
//         memcpy(dest, src + first_chunk_size, end);
//     }

//     return end;
// }
