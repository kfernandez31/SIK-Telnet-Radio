#pragma once

#include "udp_socket.hh"

#include <netinet/in.h>

#include <string>
#include <chrono>
#include <set>

#define STATION_NAME_MAX_LEN 64

struct RadioStation {
    sockaddr_in sender_addr;
    std::string mcast_addr;
    in_port_t data_port;
    std::string name;
    mutable std::chrono::steady_clock::time_point last_reply;
    
    RadioStation(
        const sockaddr_in& sender_addr,
        const std::string& mcast_addr, 
        const in_port_t& data_port,
        const std::string& name 
    );

    sockaddr_in get_data_addr() const;
    sockaddr_in get_ctrl_addr() const;
    void update_last_reply() const; // the fact this has to be const is baffling 
    
    bool operator==(const RadioStation& other) const;

    static bool is_valid_name(const std::string& name);

    struct cmp {
        bool operator()(const RadioStation& a, const RadioStation& b) const;
    };
};

using StationSet = std::set<RadioStation, RadioStation::cmp>;
