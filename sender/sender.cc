#include "sender.hh"
#include "../common/err.h"

#include <string.h>
#include <unistd.h>

struct sockaddr_in get_send_address(const char* host, const uint16_t port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *address_result;
    CHECK(getaddrinfo(host, NULL, &hints, &address_result));

    struct sockaddr_in send_address;
    send_address.sin_family = AF_INET; // IPv4
    send_address.sin_addr.s_addr = ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr; // IP address
    send_address.sin_port = htons(port);

    freeaddrinfo(address_result);
    return send_address;
}

void send_packet(int socket_fd, uint64_t session_id, uint64_t first_byte_num, const uint8_t* audio_data, const size_t psize) {
    session_id = htonll(session_id);
    first_byte_num = htonll(first_byte_num);
   
    uint8_t buffer[2 * sizeof(uint64_t) + (ssize_t)psize];
    memcpy((void*)buffer, (void*)session_id, sizeof(uint64_t));
    memcpy((void*)(buffer + sizeof(uint64_t)), (void*)first_byte_num, sizeof(uint64_t));
    memcpy((void*)(buffer + 2 * sizeof(uint64_t)), (void*)audio_data, psize);

    ssize_t sent_length = write(socket_fd, buffer, sizeof(buffer));
    VERIFY(sent_length);
    ENSURE(sent_length == sizeof(buffer));
}
