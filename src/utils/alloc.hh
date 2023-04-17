#pragma once

#include <cstdlib>

void* checked_malloc(const size_t size);
void* checked_calloc(const size_t count, const size_t size);