#pragma once

#include <cstdint>

uint64_t get_session_id(const uint8_t* pkt_buf);

uint64_t get_first_byte_num(const uint8_t* pkt_buf);

uint8_t* get_audio_data(const uint8_t* pkt_buf);