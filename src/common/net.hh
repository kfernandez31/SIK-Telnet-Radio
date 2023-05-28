#pragma once

#include <arpa/inet.h>

#include "log.hh"

#define MTU           ((1 << 16) - 1)
#define ADDR_MAX_LEN  (4 * 3 + 3)

bool operator==(const sockaddr_in& a, const sockaddr_in& b);
bool operator!=(const sockaddr_in& a, const sockaddr_in& b);


sockaddr_in get_addr(const char* host, const in_port_t port);
bool is_mcast_addr(const char* host, const in_port_t port);
