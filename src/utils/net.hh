#pragma once

#include <netdb.h>

struct sockaddr_in get_addr(const char* host, const uint16_t port);
