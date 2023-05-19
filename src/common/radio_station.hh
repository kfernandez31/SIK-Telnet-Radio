#pragma once

#include "udp_socket.hh"

#include <netinet/in.h>

#include <string>
#include <chrono>
#include <set>

#define STATION_NAME_MAX_LEN 64

struct RadioStation {
    UdpSocket data_socket;
    std::string address, name;
    in_port_t port;
    sockaddr_in data_addr, ctrl_addr;
    std::chrono::steady_clock::time_point last_reply;

    static bool is_valid_name(const std::string& name);

    RadioStation(
        const std::string& name, 
        const std::string& address, 
        const in_port_t& port, 
        const sockaddr_in& data_addr,
        const sockaddr_in& ctrl_addr
    );
    
    bool operator==(const RadioStation& other) const;

    struct cmp {
        bool operator()(const RadioStation& a, const RadioStation& b) const;
    };
};

using StationSet = std::set<RadioStation, RadioStation::cmp>;
