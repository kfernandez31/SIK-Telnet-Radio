#pragma once

#include "net.hh"

#include <netinet/in.h>

#include <cstdint>
#include <cstddef>

#include <string>
#include <memory>
#include <vector>

/// Computes the total packet size, including session ID and byte offset headers.
#define TOTAL_PSIZE(psize) ((psize) + 2 * sizeof(uint64_t))

/**
 * @struct LookupRequest
 * @brief Represents a request to discover available radio stations.
 */
struct LookupRequest {
    inline static constexpr std::string prefix = "ZERO_SEVEN_COME_IN"; ///< Fixed prefix for lookup requests.

    /**
     * @brief Constructs a `LookupRequest` from a received string.
     * @param str The received lookup request string.
     * @throws RadioException if the prefix is invalid.
     */
    LookupRequest(const std::string& str);

    LookupRequest() = default;

    /**
     * @brief Serializes the request to a string.
     * @return The serialized lookup request.
     */
    std::string to_str() const;
};

/**
 * @struct LookupReply
 * @brief Represents a response to a `LookupRequest`, providing station details.
 */
struct LookupReply {
    inline static constexpr std::string prefix = "BOREWICZ_HERE"; ///< Fixed prefix for lookup replies.

    std::string mcast_addr; ///< Multicast address for audio data.
    in_port_t data_port;    ///< Port number for receiving audio data.
    std::string name;       ///< Name of the radio station.

    /**
     * @brief Constructs a `LookupReply` from a received string.
     * @param str The received lookup reply string.
     * @throws RadioException if the format is invalid.
     */
    LookupReply(const std::string& str);

    /**
     * @brief Constructs a `LookupReply` with given parameters.
     * @param mcast_addr Multicast address of the station.
     * @param data_port Port for receiving audio data.
     * @param name Name of the radio station.
     */
    LookupReply(const std::string& mcast_addr, in_port_t data_port, const std::string& name);

    /**
     * @brief Serializes the reply to a string.
     * @return The serialized lookup reply.
     */
    std::string to_str() const;
};

/**
 * @struct RexmitRequest
 * @brief Represents a request for retransmission of lost audio packets.
 */
struct RexmitRequest {
    inline static const std::string prefix = "LOUDER_PLEASE"; ///< Fixed prefix for retransmission requests.

    sockaddr_in receiver_addr;        ///< Address of the requesting client.
    std::vector<uint64_t> packet_ids; ///< List of missing packet IDs.

    /**
     * @brief Constructs a `RexmitRequest` from a received string.
     * @param receiver_addr Address of the requesting client.
     * @param str The received retransmission request string.
     * @throws RadioException if the format is invalid.
     */
    RexmitRequest(const sockaddr_in& receiver_addr, const std::string& str);

    /**
     * @brief Constructs a `RexmitRequest` with specified parameters.
     * @param receiver_addr Address of the requesting client.
     * @param packet_ids List of missing packet IDs.
     */
    RexmitRequest(const sockaddr_in& receiver_addr, const std::vector<uint64_t>& packet_ids);

    /// Move constructor.
    RexmitRequest(RexmitRequest&& other);

    /**
     * @brief Serializes the request to a string.
     * @return The serialized retransmission request.
     */
    std::string to_str() const;
};

/**
 * @struct AudioPacket
 * @brief Represents an audio packet containing streaming data.
 */
struct AudioPacket {
    uint64_t session_id;           ///< Unique session identifier.
    uint64_t first_byte_num;       ///< Byte offset of the first audio byte.
    size_t psize;                  ///< Size of the audio payload.
    std::unique_ptr<char[]> bytes; ///< Pointer to raw packet data.

    /**
     * @brief Constructs an `AudioPacket` from a received buffer.
     * @param buf The received packet buffer.
     * @param psize The size of the audio payload.
     */
    AudioPacket(const char* buf, size_t psize);

    /**
     * @brief Constructs an `AudioPacket` for sending.
     * @param session_id The session identifier.
     * @param first_byte_num The byte offset of the first audio byte.
     * @param audio_data_buf The buffer containing the audio payload.
     * @param psize The size of the audio payload.
     */
    AudioPacket(uint64_t session_id, uint64_t first_byte_num, const char* audio_data_buf, size_t psize);

    /// Move constructor.
    AudioPacket(AudioPacket&& other);

    /**
     * @brief Returns a pointer to the audio payload.
     * @return Pointer to the audio data.
     */
    const char* audio_data() const;

    /// Maximum size of an audio packet payload.
    inline static constexpr size_t MAX_PSIZE = UDP_MAX_DATA_SIZE - 2 * sizeof(uint64_t);
};

/**
 * @enum DatagramType
 * @brief Identifies the type of datagram being processed.
 */
enum class DatagramType {
    None,           ///< Undefined or unrecognized datagram.
    LookupRequest,  ///< Lookup request datagram.
    LookupReply,    ///< Lookup reply datagram.
    RexmitRequest,  ///< Retransmission request datagram.
    AudioPacket,    ///< Audio data packet.
};