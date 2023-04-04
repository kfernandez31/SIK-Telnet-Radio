#pragma once

#include <stdint.h>

//TODO: remove

struct __attribute__((__packed__)) audio_packet {
    uint64_t session_id;
    uint64_t first_byte_num;
    uint8_t audio_data[];
};
