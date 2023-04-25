#pragma once

#include <cstddef>
#include <cstdint>

class cyclical_buffer {
public:
    cyclical_buffer(const size_t capacity);
    ~cyclical_buffer();

    void   dump(const size_t nbytes);
    void   fill_gap(const uint8_t* src, const size_t offset);
    void   reset(const size_t psize);
    size_t range() const;

    void   advance(const uint8_t* src, const size_t offset);
 
    size_t rounded_cap() const;   

    uint8_t* data;
    bool*    populated;
    size_t   capacity, tail, head, psize;
private:
    enum side  { NONE, LEFT, RIGHT, };
    side   sideof(const size_t idx) const;
};