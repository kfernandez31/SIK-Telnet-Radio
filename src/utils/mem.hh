#pragma once

#include <cstdint>
#include <cstddef>

uint8_t* my_memcpy(uint8_t* buffer, const void* src, const size_t nbytes);
void my_write(const int fd, const void* src, const size_t nbytes);