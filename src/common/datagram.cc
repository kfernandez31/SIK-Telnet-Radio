#include "datagram.hh"

#include "radio_station.hh"
#include "net.hh"
#include "endian.hh"
#include "except.hh"

#include <cctype>

#include <sstream>
#include <algorithm>
#include <iterator>

#define FIELD_SEPARATOR  ' '
#define PACKET_SEPARATOR ","

static inline std::string trimmed(std::string str) {
    if (!str.empty() && str.back() == '\n')
        str.pop_back();
    return str;
}

/// Checks if a string consists only of numeric characters.
static inline bool is_numeric(const std::string& str) {
    return std::all_of(str.begin(), str.end(), std::isdigit);
}

//----------------------------LookupRequest------------------------------------

LookupRequest::LookupRequest(const std::string& str) {
    if (trimmed(str) != LookupRequest::prefix)
        throw RadioException("Invalid prefix");
}

std::string LookupRequest::to_str() const {
    return LookupRequest::prefix + '\n';
}

//----------------------------LookupReply------------------------------------

// note to self: parsing would've been better with strtok...

LookupReply::LookupReply(const std::string& str) {
    std::string input = trimmed(str);

    // prefix
    size_t space_pos = input.find(FIELD_SEPARATOR);
    if (space_pos != LookupReply::prefix.length())
        throw RadioException("Invalid format");
    if (input.substr(0, space_pos) != LookupReply::prefix)
        throw RadioException("Invalid prefix");
    input = input.substr(space_pos + 1);

    // mcast_addr
    space_pos = input.find(FIELD_SEPARATOR);
    if (space_pos == std::string::npos)
        throw RadioException("Invalid format");
    mcast_addr = input.substr(0, space_pos);
    input = input.substr(space_pos + 1);

    // data_port
    space_pos = input.find(FIELD_SEPARATOR);
    if (space_pos == std::string::npos)
        throw RadioException("Invalid format");
    unsigned long value = std::stoul(input.substr(0, space_pos));
    if (value > std::numeric_limits<uint16_t>::max())
        throw RadioException("Invalid data_port");
    data_port = static_cast<uint16_t>(value);
    if (!get_mcast_addr(mcast_addr.c_str(), data_port))
        throw RadioException("Invalid mcast_addr");
    input = input.substr(space_pos + 1);

    // name
    name = input;
    if (!RadioStation::is_valid_name(name))
        throw RadioException("Invalid name");
}

LookupReply::LookupReply(const std::string& mcast_addr, const in_port_t data_port, const std::string& name)
    : mcast_addr(mcast_addr), data_port(data_port), name(name) {}

std::string LookupReply::to_str() const {
    return
        LookupReply::prefix + FIELD_SEPARATOR +
        mcast_addr + FIELD_SEPARATOR +
        std::to_string(data_port) + FIELD_SEPARATOR +
        name + '\n';
}

//----------------------------RexmitRequest------------------------------------

RexmitRequest::RexmitRequest(const sockaddr_in& receiver_addr, const std::string& str)
    : receiver_addr(receiver_addr)
{
    std::string input = trimmed(str);

    if (!isdigit(input.back()) && input.back() != FIELD_SEPARATOR)
        throw RadioException("Invalid last character");

    // prefix
    size_t space_pos = input.find(FIELD_SEPARATOR);
    if (space_pos != RexmitRequest::prefix.length())
        throw RadioException("Invalid format");
    if (input.substr(0, space_pos) != RexmitRequest::prefix)
        throw RadioException("Invalid prefix");

    // packet ids
    input = input.substr(space_pos + 1);
    std::istringstream ss(input);
    std::string token;
    while (std::getline(ss, token, *PACKET_SEPARATOR)) {
        if (!is_numeric(token))
            throw RadioException("Invalid packet ids");
        packet_ids.push_back(std::stoull(token));
    }
}

RexmitRequest::RexmitRequest(RexmitRequest&& other)
    : receiver_addr(other.receiver_addr)
    , packet_ids(std::move(other.packet_ids))
    {}

RexmitRequest::RexmitRequest(const sockaddr_in& receiver_addr, const std::vector<uint64_t>& packet_ids)
    : receiver_addr(receiver_addr)
    , packet_ids(packet_ids)
    {}

std::string RexmitRequest::to_str() const {
    std::ostringstream oss;
    // prefix
    oss << RexmitRequest::prefix << FIELD_SEPARATOR;

    // packet_ids
    if (!packet_ids.empty()) {
        oss << packet_ids.front();
        for (size_t i = 1; i < packet_ids.size(); ++i)
            oss << PACKET_SEPARATOR << packet_ids[i];
    }
    oss << '\n';
    return oss.str();
}

//----------------------------AudioPacket------------------------------------

// used for receiving
AudioPacket::AudioPacket(const char* buf, const size_t psize)
    : session_id(ntohll(((uint64_t*)buf)[0]))
    , first_byte_num(ntohll(((uint64_t*)buf)[1]))
    , psize(psize)
{
    bytes = std::make_unique<char[]>(TOTAL_PSIZE(psize));
    memcpy(bytes.get(), buf, TOTAL_PSIZE(psize));
}

// used for sending
AudioPacket::AudioPacket(const uint64_t session_id, const uint64_t first_byte_num, const char* audio_data, const size_t psize)
    : session_id(session_id)
    , first_byte_num(first_byte_num)
    , psize(psize)
{
    bytes = std::make_unique<char[]>(TOTAL_PSIZE(psize));
    uint64_t val = htonll(session_id);
    memcpy(bytes.get(), &val, sizeof(val));
    val = htonll(first_byte_num);
    memcpy(bytes.get() + sizeof(val), &val, sizeof(val));
    memcpy(bytes.get() + 2 * sizeof(val), audio_data, psize);
}

AudioPacket::AudioPacket(AudioPacket&& other)
    : session_id(other.session_id)
    , first_byte_num(other.first_byte_num)
    , bytes(std::move(other.bytes))
    {}

const char* AudioPacket::audio_data() const {
    return bytes.get() + 2 * sizeof(uint64_t);
}
