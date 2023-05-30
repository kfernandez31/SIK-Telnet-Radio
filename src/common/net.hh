#pragma once

#include <arpa/inet.h>

#include "log.hh"

#include <optional>

#define UDP_MAX_DATA_SIZE           ((1 << 16) - 1) // hope this is a good assumption
#define ADDR_MAX_LEN  (4 * 3 + 3)

bool operator==(const sockaddr_in& a, const sockaddr_in& b);
bool operator!=(const sockaddr_in& a, const sockaddr_in& b);
bool operator<(const sockaddr_in& a, const sockaddr_in& b);

sockaddr_in get_addr(const char* host, const in_port_t port);
std::optional<sockaddr_in> get_mcast_addr(const char* host, const in_port_t port);
