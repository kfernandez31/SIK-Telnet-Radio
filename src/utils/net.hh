#pragma once

#include <netdb.h>
#include <optional>

struct sockaddr_in get_addr(const char* host, const std::optional<uint16_t> port);
