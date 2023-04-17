#pragma once

#include <cstddef>
#include <cstdint>

uint8_t* bufcpy(uint8_t* buffer, const void* src, const size_t nbytes);

size_t cyclical_write(const int fd, const uint8_t* buf, const size_t bsize, const size_t start, const size_t nbytes);

size_t cyclical_memcpy(uint8_t* dest, const uint8_t* src, const size_t bsize, const size_t start, const size_t nbytes);

bool is_between(const size_t tail, const size_t head, const size_t bsize, const size_t idx);
