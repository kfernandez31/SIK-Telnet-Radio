#pragma once

#include "datagram.hh"

#include <netinet/in.h>

#include <string>
#include <chrono>
#include <set>

#define STATION_NAME_MAX_LEN 64 ///< Maximum allowed length for a station name.

/**
 * @class RadioStation
 * @brief Represents a radio station in a multicast network.
 *
 * This struct encapsulates the essential details of a radio station,
 * including its control, data, and multicast addresses. It also tracks
 * the station's name and the last received reply.
 */
struct RadioStation {
    sockaddr_in data_addr;                                    ///< Address used for data transmission.
    sockaddr_in ctrl_addr;                                    ///< Control address (origin of discovery messages).
    sockaddr_in mcast_addr;                                   ///< Multicast address for receiving broadcast data.
    std::string name;                                         ///< Name of the radio station.
    mutable std::chrono::steady_clock::time_point last_reply; ///< Timestamp of the last received reply.

    /**
     * @brief Constructs a `RadioStation` from a sender address and a lookup reply.
     * @param sender_addr The address of the sender (control).
     * @param payload The received `LookupReply` containing station details.
     * @throws RadioException if the station name is invalid.
     */
    RadioStation(const sockaddr_in& sender_addr, const LookupReply& payload);

    /**
     * @brief Updates the last reply timestamp to the current time.
     */
    void update_last_reply() const;

    /**
     * @brief Compares two `RadioStation` objects for equality.
     * @param other The station to compare against.
     * @return True if both stations have the same name and addresses.
     */
    bool operator==(const RadioStation& other) const;

    /**
     * @brief Validates a station name based on specific criteria.
     * @param name The station name to validate.
     * @return True if the name is valid, false otherwise.
     */
    static bool is_valid_name(const std::string& name);

    /**
     * @struct cmp
     * @brief Comparator for ordering `RadioStation` objects.
     *
     * Compares radio stations based on their name, then multicast address, and finally data address.
     */
    struct cmp {
        bool operator()(const RadioStation& a, const RadioStation& b) const;
    };
};

/**
 * @typedef StationSet
 * @brief Represents a set of unique `RadioStation` objects.
 *
 * Uses `RadioStation::cmp` for ordering stations based on name, multicast address, and data address.
 */
using StationSet = std::set<RadioStation, RadioStation::cmp>;
