#pragma once

#include "net.hh"

#include <netinet/in.h>

#include <cstdint>
#include <cstddef>

#include <string>
#include <vector>

#define LOOKUP_REQUEST_PREFIX "ZERO_SEVEN_COME_IN"
#define LOOKUP_REPLY_PREFIX   "BOREWICZ_HERE"
#define REXMIT_REQUEST_PREFIX "LOUDER_PLEASE"
#define PACKET_SEPARATOR      ","

#define UINT16_T_LEN          5
#define UINT32_T_LEN          10
#define MCAST_ADDR_LEN        15
#define LOOKUP_REPLY_MAX_LEN  (sizeof(LOOKUP_REPLY_PREFIX)-1 + 1 + MCAST_ADDR_LEN + 1 + UINT16_T_LEN + 1 + STATION_NAME_MAX_LEN)

#define TOTAL_PSIZE(psize)    ((psize) + 2 * sizeof(uint64_t))

struct LookupRequest {
    static const std::string prefix;

    LookupRequest(const std::string& str);
    LookupRequest() {}

    std::string to_str() const;
};

struct LookupReply {
    static const std::string prefix;
    std::string mcast_addr;
    in_port_t data_port;
    std::string name;

    LookupReply(const std::string& str);
    LookupReply(const std::string& mcast_addr, const in_port_t data_port, const std::string& name);
    std::string to_str() const;
};

struct RexmitRequest {
    static const std::string prefix;

    sockaddr_in receiver_addr;
    std::vector<uint64_t> packet_ids;

    RexmitRequest(const std::string& str);
    RexmitRequest(const std::vector<uint64_t>& packet_ids);

    std::string to_str() const;
};

struct AudioPacket {
    AudioPacket(const char* buf);
    AudioPacket(const uint64_t first_byte_num, const uint64_t session_id, const char* audio_data_buf, const size_t psize);

    std::vector<char> bytes;
    uint64_t session_id, first_byte_num;
    const char* audio_data() const;
    size_t total_size() const;
    size_t psize() const;

    static const size_t MAX_PSIZE = MTU - 2 * sizeof(uint64_t);
};
