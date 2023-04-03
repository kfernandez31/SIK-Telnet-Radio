#pragma once

#include <netdb.h>

struct sockaddr_in get_send_address(const char* host, const uint16_t port);
void send_packet(int socket_fd, uint64_t session_id, uint64_t first_byte_num, const uint8_t* audio_data, const size_t psize);