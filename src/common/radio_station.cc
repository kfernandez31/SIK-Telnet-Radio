#include "radio_station.hh"
#include "except.hh"

#include <string>
#include <chrono>
#include <netinet/in.h>

#include "net.hh"

RadioStation::RadioStation(
    const sockaddr_in& sender_addr,
    const std::string& mcast_addr, 
    const in_port_t& data_port,
    const std::string& name 
) 
    : sender_addr(sender_addr)
    , mcast_addr(mcast_addr)
    , data_port(data_port)
    , name(name)
    , last_reply(std::chrono::steady_clock::now()) 
{
    if (!is_valid_name(name))
        throw RadioException("Invalid name");
}

sockaddr_in RadioStation::get_data_addr() const {
    return get_addr(mcast_addr.c_str(), data_port);
}

sockaddr_in RadioStation::get_ctrl_addr() const {
    return get_addr(mcast_addr.c_str(), ntohs(sender_addr.sin_port));
}

bool RadioStation::operator==(const RadioStation& other) const {
    return name == other.name && sender_addr == other.sender_addr && data_port == other.data_port;
}

bool RadioStation::cmp::operator()(const RadioStation& a, const RadioStation& b) const {
    if (a.name != b.name)
        return a.name < b.name;
    if (a.data_port != b.data_port)
        return a.data_port < b.data_port;
    return a.sender_addr != b.sender_addr;
}

bool RadioStation::is_valid_name(const std::string& name) {
    if (name.empty() || name.front() == ' ' || name.back() == ' ' || name.size() > STATION_NAME_MAX_LEN)
        return false;
    for (const char c : name)
        if (c < 32 || c > 127)
            return false;
    return true;
}
