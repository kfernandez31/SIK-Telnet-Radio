#pragma once

#include "datagram.hh"

#include <cstddef>
#include <cstdint>

class CircularBuffer {
public:
    CircularBuffer(const size_t capacity);
    ~CircularBuffer();

    void reset(const size_t psize);
    void reset(const size_t psize, const uint64_t abs_head_);

    void dump_tail(const size_t nbytes);
    void try_put(const AudioPacket& packet); // the most used method, "push"
    size_t cnt_upto_gap() const;
    size_t range() const;
    size_t capacity() const;
    size_t rounded_cap() const;   
    size_t psize() const;
    size_t tail() const;
    size_t head() const;
    char* data() const;
    bool occupied(const size_t idx) const;
    uint64_t abs_head() const;
    uint64_t abs_tail() const;
    uint64_t byte0() const;
private:
    uint64_t _abs_head, _byte0;
    size_t _capacity, _psize, _tail, _head;
    bool*  _occupied;
    char* _data;

    enum side  { NONE, LEFT, RIGHT, };
    side sideof(const size_t idx) const;
    void fill_gap(const AudioPacket& packet);
    void try_push_head(const AudioPacket& packet);
};
