#pragma once

#include "net.hh"

#include <netinet/in.h>

#include <cstdint>
#include <cstddef>

#include <string>
#include <vector>

#define TOTAL_PSIZE(psize)    ((psize) + 2 * sizeof(uint64_t))

struct LookupRequest {
    inline static const std::string prefix = "ZERO_SEVEN_COME_IN";

    LookupRequest(const std::string& str);
    LookupRequest() {}

    std::string to_str() const;
};

struct LookupReply {
    inline static const std::string prefix = "BOREWICZ_HERE";

    std::string mcast_addr;
    in_port_t data_port;
    std::string name;

    LookupReply(const std::string& str);
    LookupReply(const std::string& mcast_addr, const in_port_t data_port, const std::string& name);
    
    std::string to_str() const;
};

struct RexmitRequest {
    inline static const std::string prefix = "LOUDER_PLEASE";
    
    sockaddr_in receiver_addr;
    std::vector<uint64_t> packet_ids;

    RexmitRequest(const std::string& str);
    RexmitRequest(RexmitRequest&& other);
    RexmitRequest(const std::vector<uint64_t>& packet_ids);

    std::string to_str() const;
};

struct AudioPacket {
    AudioPacket(const char* buf, const size_t psize);
    AudioPacket(const uint64_t session_id, const uint64_t first_byte_num, const char* audio_data_buf, const size_t psize);
    AudioPacket(AudioPacket&& other);

    uint64_t session_id, first_byte_num;
    size_t psize;
    std::unique_ptr<char[]> bytes;

    const char* audio_data() const;

    static const size_t MAX_PSIZE = UDP_MAX_DATA_SIZE - 2 * sizeof(uint64_t);
};

enum class DatagramType {
    None,
    LookupRequest,
    LookupReply,
    RexmitRequest,
    AudioPacket,
};
