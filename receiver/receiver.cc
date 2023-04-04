#include "receiver.hh"
#include "../common/err.h"

#include <string.h>
#include <unistd.h>

int bind_socket(const uint16_t port) {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    ENSURE(socket_fd > 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(port);

    // bind the socket to a concrete address
    CHECK_ERRNO(bind(socket_fd, (struct sockaddr*)&server_address, (socklen_t)sizeof(server_address)));
    return socket_fd;
}

void read_packet(const int socket_fd, struct sockaddr_in* client_address, audio_packet* pkt, const size_t pkt_size) {
    socklen_t addr_len = (socklen_t) sizeof(*client_address);
    ssize_t nread = recvfrom(socket_fd, (void*)pkt, pkt_size, 0, (struct sockaddr*)client_address, &addr_len);
    ENSURE(nread == pkt_size);
}

uint64_t write_threshold(const uint64_t byte0, const size_t bsize) {
    return byte0 + (uint64_t)(bsize * 3 / 4);
}

void dump_buffer(const uint8_t* buffer, const size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%u", buffer[i]);
    }
}
