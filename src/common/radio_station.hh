#pragma once

#include "datagram.hh"

#include <netinet/in.h>

#include <string>
#include <chrono>
#include <set>

#define STATION_NAME_MAX_LEN 64

struct RadioStation {
    sockaddr_in data_addr, ctrl_addr, mcast_addr;
    std::string name;
    mutable std::chrono::steady_clock::time_point last_reply;
    
    RadioStation(
        const sockaddr_in& sender_addr,
        const LookupReply& payload
    );

    void update_last_reply() const;
    
    bool operator==(const RadioStation& other) const;

    static bool is_valid_name(const std::string& name);

    struct cmp {
        bool operator()(const RadioStation& a, const RadioStation& b) const;
    };
};

using StationSet = std::set<RadioStation, RadioStation::cmp>;
