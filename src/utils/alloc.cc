#include "./alloc.hh"

#include <cstdio>

void* checked_malloc(const size_t size) {
    void* p = malloc(size);
    if (p == NULL) {
        fprintf(stderr, "malloc: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

void* checked_calloc(const size_t count, const size_t size) {
    void* p = calloc(count, size);
    if (p == NULL) {
        fprintf(stderr, "calloc: out of memory\n");
        exit(EXIT_FAILURE);
    }
    return p;
}
