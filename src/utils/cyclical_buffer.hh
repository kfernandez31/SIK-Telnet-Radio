#pragma once

#include <cstddef>
#include <cstdint>

class cyclical_buffer {
public:
    cyclical_buffer(const size_t capacity);
    ~cyclical_buffer();

    void   reset(const size_t psize);
    void   pop_tail(uint8_t* dst, const size_t nbytes);
    void   pop_tail(uint8_t* dst);
    void   fill_gap(const uint8_t* src, const size_t offset);
    void   push_head(const uint8_t* src, const size_t offset);
    size_t range() const;
    bool   empty() const;
    size_t rounded_cap() const;   
    size_t psize() const;
    size_t tail() const;
    size_t head() const;
    bool   occupied(const size_t idx) const;
private:
    size_t _capacity, _psize, _tail, _head;
    bool*  _occupied;
    uint8_t* _data;

    enum side  { NONE, LEFT, RIGHT, };

    side   sideof(const size_t idx) const;
};