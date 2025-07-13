#pragma once

#include "datagram.hh"

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * @class CircularBuffer
 * @brief A circular buffer designed for audio packet storage and playback.
 *
 * This buffer efficiently manages incoming audio packets, handling packet gaps and
 * ensuring proper ordering using a fixed-size memory allocation.
 */
class CircularBuffer {
public:
    CircularBuffer(size_t capacity);

    ~CircularBuffer();

    /**
     * @brief Resets the buffer with a given packet size.
     * @param psize Size of an individual audio packet.
     */
    void reset(size_t psize);
    /**
     * @brief Resets the buffer with a given packet size and absolute head position.
     * @param psize Size of an individual audio packet.
     * @param abs_head The new absolute head position.
     */
    void reset(size_t psize, uint64_t abs_head);

    /**
     * @brief Dumps a portion of the buffer to standard output.
     * @param nbytes Number of bytes to dump.
     */
    void dump_tail(size_t nbytes);

    /**
     * @brief Inserts an audio packet into the buffer.
     * @param packet The packet to insert.
     */
    void try_put(const AudioPacket& packet);

    /// @return The number of continuous occupied packets from the tail.
    size_t cnt_upto_gap() const;

    /// @return The number of bytes currently stored in the buffer.
    size_t range() const;

    /// @return The total capacity of the buffer.
    size_t capacity() const;

    /// @return The rounded capacity, ensuring packet alignment.
    size_t rounded_cap() const;

    /// @return The size of an individual packet.
    size_t psize() const;

    /// @return The current tail position in the buffer.
    size_t tail() const;

    /// @return The current head position in the buffer.
    size_t head() const;

    /// @return A pointer to the internal buffer data.
    char* data() const;

    /**
     * @brief Checks if a specific buffer index is occupied.
     * @param idx The index to check.
     * @return True if occupied, false otherwise.
     */
    bool occupied(size_t idx) const;

    /// @return The absolute head position of the buffer.
    uint64_t abs_head() const;

    /// @return The absolute tail position of the buffer.
    uint64_t abs_tail() const;

    /// @return The starting byte offset of the buffer.
    uint64_t byte0() const;

    /// @return The printing threshold, used for debugging/logging.
    uint64_t printing_threshold() const;

    /// @return True if the buffer is empty, false otherwise.
    bool empty() const;

private:
    uint64_t _abs_head;  ///< Absolute head position of the buffer.
    uint64_t _byte0;     ///< The starting byte offset.
    size_t _capacity;    ///< Total capacity of the buffer.
    size_t _psize;       ///< Size of a single packet.
    size_t _tail;        ///< Tail index of the buffer.
    size_t _head;        ///< Head index of the buffer.
    char* _data;         ///< Pointer to the buffer data.
    bool* _occupied;     ///< Array tracking occupied positions.
    bool _empty;         ///< Flag indicating if the buffer is empty.

    /// Enum to determine which side of the buffer an index belongs to.
    enum side { NONE, LEFT, RIGHT };

    /**
     * @brief Determines which side an index belongs to in the buffer.
     * @param idx The index to check.
     * @return LEFT, RIGHT, or NONE.
     */
    side sideof(size_t idx) const;

    /**
     * @brief Fills gaps in the buffer with a missing packet.
     * @param packet The packet to fill the gap.
     */
    void fill_gap(const AudioPacket& packet);

    /**
     * @brief Attempts to push an audio packet to the head of the buffer.
     * @param packet The packet to insert.
     */
    void try_push_head(const AudioPacket& packet);
};