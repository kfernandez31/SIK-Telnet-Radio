#include "radio_station.hh"
#include "except.hh"

#include <string>
#include <chrono>

#include <netinet/in.h>

#include "net.hh"

RadioStation::RadioStation(
    const sockaddr_in& sender_addr,
    const LookupReply& payload
)
    : ctrl_addr(sender_addr)
    , name(payload.name)
    , last_reply(std::chrono::steady_clock::now())
{
    if (!is_valid_name(name))
        throw RadioException("Invalid name");

    mcast_addr = get_addr(payload.mcast_addr.c_str(), payload.data_port);

    data_addr = ctrl_addr;
    data_addr.sin_port = htons(payload.data_port);
}

void RadioStation::update_last_reply() const {
    last_reply = std::chrono::steady_clock::now();
}

bool RadioStation::operator==(const RadioStation& other) const {
    return  name == other.name
        && mcast_addr == other.mcast_addr
        && data_addr  == other.data_addr;
}

bool RadioStation::cmp::operator()(const RadioStation& a, const RadioStation& b) const {
    return std::tie(a.name, a.mcast_addr, a.data_addr) < std::tie(b.name, b.mcast_addr, b.data_addr);
}

bool RadioStation::is_valid_name(const std::string& name) {
    if (name.empty() || name.front() == ' ' || name.back() == ' ' || name.size() > STATION_NAME_MAX_LEN)
        return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char c) { return c >= 32 && c <= 127; });
}
