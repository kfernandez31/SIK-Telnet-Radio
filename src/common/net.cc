#include "net.hh"
#include "err.hh"

#include <netdb.h>

bool operator==(const sockaddr_in& a, const sockaddr_in& b) {
    if (a.sin_addr.s_addr != b.sin_addr.s_addr)
        return false;
    return a.sin_port == b.sin_port && a.sin_family == b.sin_family;
}

bool operator!=(const sockaddr_in& a, const sockaddr_in& b) {
    return !(a == b);
}

sockaddr_in get_addr(const char* host, const in_port_t port) {
    addrinfo hints = {};
    hints.ai_family   = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    addrinfo* addr_info;
    if (0 != getaddrinfo(host, NULL, &hints, &addr_info))
        fatal("getaddrinfo");

    sockaddr_in addr;
    addr.sin_family      = AF_INET; // IPv4
    addr.sin_addr.s_addr = ((sockaddr_in*)(addr_info->ai_addr))->sin_addr.s_addr; // IP address
    addr.sin_port        = htons(port);

    freeaddrinfo(addr_info);
    return addr;
}

bool is_valid_mcast_addr(const char* str) {
    struct in_addr addr;
    int result = inet_pton(AF_INET, str, &addr);
    return result == 1 && IN_MULTICAST(ntohl(addr.s_addr));
}
