#pragma once

#include <arpa/inet.h>

#include "log.hh"

#include <optional>

/**
 * @def UDP_MAX_DATA_SIZE
 * @brief Defines the maximum UDP data size.
 *
 * Assumes the maximum possible UDP payload size, based on a 16-bit length field.
 */
#define UDP_MAX_DATA_SIZE ((1 << 16) - 1) ///< Maximum UDP payload size (65535 bytes).

/**
 * @def ADDR_MAX_LEN
 * @brief Maximum length of an IPv4 address as a string.
 *
 * An IPv4 address in dot-decimal notation consists of four numbers (0-255) separated by three dots,
 * requiring at most "255.255.255.255" (15 characters, including null terminator).
 */
#define ADDR_MAX_LEN (4 * 3 + 3) ///< Maximum length for an IPv4 address string.

bool operator==(const sockaddr_in& a, const sockaddr_in& b);

bool operator!=(const sockaddr_in& a, const sockaddr_in& b);

bool operator<(const sockaddr_in& a, const sockaddr_in& b);

/**
 * @brief Resolves a hostname to an IPv4 address.
 *
 * Uses `getaddrinfo()` to resolve the given hostname and constructs a `sockaddr_in` structure
 * with the resolved IP and the specified port.
 *
 * @param host The hostname or IP address string.
 * @param port The port number to associate with the address.
 * @return A `sockaddr_in` structure representing the resolved address.
 * @throws Calls `fatal()` if name resolution fails.
 */
sockaddr_in get_addr(const char* host, const in_port_t port);

/**
 * @brief Resolves a hostname to a multicast IPv4 address.
 *
 * If the provided hostname is a valid multicast address, returns an optional `sockaddr_in`.
 * Otherwise, returns an empty optional.
 *
 * @param host The multicast address string.
 * @param port The port number to associate with the address.
 * @return `std::optional<sockaddr_in>` containing the address if valid, or an empty optional.
 */
std::optional<sockaddr_in> get_mcast_addr(const char* host, const in_port_t port);
