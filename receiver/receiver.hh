#pragma once

#include "../common/audio_packet.hh"

#include <netdb.h>
#include <memory>

int bind_socket(const uint16_t port);

void read_packet(const int socket_fd, struct sockaddr_in* client_address, audio_packet* pkt, const size_t pkt_size);

uint64_t write_threshold(const uint64_t byte0, const size_t bsize);

void dump_buffer(const uint8_t* buffer, const size_t len);
