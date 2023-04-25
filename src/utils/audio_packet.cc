#include "./audio_packet.hh"
#include "./endian.hh"

uint64_t get_session_id(const uint8_t* pkt_buf) {
    return ntohll(((uint64_t*)pkt_buf)[0]);
}

uint64_t get_first_byte_num(const uint8_t* pkt_buf) {
    return ntohll(((uint64_t*)pkt_buf)[1]);
}

uint8_t* get_audio_data(const uint8_t* pkt_buf) {
    return const_cast<uint8_t*>(pkt_buf) + 2 * sizeof(uint64_t);
}
