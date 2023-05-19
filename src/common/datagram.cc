#include "datagram.hh"

#include "radio_station.hh"
#include "net.hh"
#include "except.hh"

#include <sstream>
#include <algorithm>
#include <iterator>

//----------------------------LookupRequest------------------------------------

LookupRequest::LookupRequest(const std::string& str) {
    if (str != LOOKUP_REQUEST_PREFIX)
        throw RadioException("Invalid prefix");
}

std::string LookupRequest::to_str() const {
    return LOOKUP_REQUEST_PREFIX;
}

//----------------------------LookupReply------------------------------------

LookupReply::LookupReply(const std::string& str) {
    std::istringstream ss(str);
    std::string prefix;
    ss >> prefix;
    if (prefix != LOOKUP_REPLY_PREFIX)
        throw RadioException("Invalid prefix");
    ss >> mcast_addr;
    if (!is_valid_mcast_addr(mcast_addr.c_str()))
        throw RadioException("Invalid multicast address");
    try {
        ss >> data_port;
    } catch (std::exception&) {
        throw RadioException("Invalid data port");
    }
    ss >> name;
    if (RadioStation::is_valid_name(name))
        throw RadioException("Invalid name");
}

LookupReply::LookupReply(const std::string& mcast_addr, const in_port_t data_port, const std::string& name)
    : mcast_addr(mcast_addr), data_port(data_port), name(name) {}

std::string LookupReply::to_str() const {
    return std::string(LOOKUP_REPLY_PREFIX) + " " + mcast_addr + " " + std::to_string(data_port) + " " + name;
}

//----------------------------RexmitRequest------------------------------------

RexmitRequest::RexmitRequest(const std::string& str) {
    std::istringstream ss(str);
    std::string prefix;
    if (prefix != REXMIT_REQUEST_PREFIX)
        throw RadioException("Invalid prefix");

    std::string token;
    while (std::getline(ss, token, *PACKET_SEPARATOR))
        try {
            packet_ids.push_back(std::stoull(token));
        } catch (const std::exception& e) {
            throw RadioException("Invalid packet ids format");
        }
}

RexmitRequest::RexmitRequest(const std::vector<uint64_t>& packet_ids)
    : packet_ids(packet_ids) {}

std::string RexmitRequest::to_str() const {
    std::ostringstream oss;
    std::copy(packet_ids.begin(), std::prev(packet_ids.end()), std::ostream_iterator<uint64_t>(oss, PACKET_SEPARATOR));
    if (!packet_ids.empty())
        oss << packet_ids.back();
    return oss.str();
}

//----------------------------AudioPacket------------------------------------

// used for receiving
AudioPacket::AudioPacket(const char* buf)
    : session_id(ntohll(((uint64_t*)buf)[0]))
    , first_byte_num(ntohll(((uint64_t*)buf)[1]))
{
    bytes.push_back(session_id);
    bytes.push_back(first_byte_num);
    const char* audio_bytes = buf + 2 * sizeof(uint64_t);
    size_t psize_ = strlen(audio_bytes);
    for (size_t i = 0; i < psize_; ++i)
        bytes.push_back(audio_bytes[i]);
}

// used for sending
AudioPacket::AudioPacket(const uint64_t first_byte_num, const uint64_t session_id, const char* audio_data_buf, const size_t psize) {
    bytes.push_back(htonll(session_id));
    bytes.push_back(htonll(first_byte_num));
    for (size_t i = 0; i < psize; ++i)
        bytes.push_back(audio_data_buf[i]);
}

const char* AudioPacket::audio_data() const {
    return bytes.data() + 2 * sizeof(uint64_t);
}

size_t AudioPacket::psize() const {
    return bytes.size();
}

size_t AudioPacket::total_size() const {
    return TOTAL_PSIZE(psize());
}
