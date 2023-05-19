#include "radio_station.hh"
#include "except.hh"

#include <string>
#include <chrono>
#include <netinet/in.h>

RadioStation::RadioStation(
    const std::string& name, 
    const std::string& address, 
    const in_port_t& port, 
    const sockaddr_in& data_addr,
    const sockaddr_in& ctrl_addr
) : address(address)
  , port(port)
  , data_addr(data_addr)
  , ctrl_addr(ctrl_addr)
  , last_reply(std::chrono::steady_clock::now()) 
{
    //TODO: init socket
    if (!is_valid_name(name))
        throw RadioException("Invalid name");
    this->name = name;
}

bool RadioStation::operator==(const RadioStation& other) const {
    return name == other.name && address == other.address && port == other.port;
}

bool RadioStation::cmp::operator()(const RadioStation& a, const RadioStation& b) const {
    if (a.name != b.name)
        return a.name < b.name;
    if (a.address != b.address)
        return a.address < b.address;
    return a.port < b.port;
}

bool RadioStation::is_valid_name(const std::string& name) {
    if (name.empty() || name.front() == ' ' || name.back() == ' ' || name.size() > STATION_NAME_MAX_LEN)
        return false;
    for (const char c : name)
        if (c < 32 || c > 127)
            return false;
    return true;
}
