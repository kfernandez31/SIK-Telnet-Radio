#include "./net.hh"
#include "./err.hh"

#include <arpa/inet.h>

struct sockaddr_in get_addr(const char* host, const uint16_t port) {
    struct addrinfo hints = {};
    hints.ai_family   = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *addr_info;
    if (0 != getaddrinfo(host, NULL, &hints, &addr_info))
        fatal("getaddrinfo");

    struct sockaddr_in addr;
    addr.sin_family      = AF_INET; // IPv4
    addr.sin_addr.s_addr = ((struct sockaddr_in *) (addr_info->ai_addr))->sin_addr.s_addr; // IP address
    addr.sin_port        = htons(port);

    freeaddrinfo(addr_info);
    return addr;
}
