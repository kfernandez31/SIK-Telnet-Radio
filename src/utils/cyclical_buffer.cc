#include "./cyclical_buffer.hh"
#include "./err.hh"

#include <unistd.h>
#include <cstring>

#include <algorithm>

cyclical_buffer::cyclical_buffer(const size_t capacity) 
    : _capacity(capacity), _psize(0), _tail(0), _head(0)
{
    _data     = new uint8_t[capacity]();
    _occupied = new bool[capacity]();
}

size_t cyclical_buffer::rounded_cap() const {
    return _capacity / _psize * _psize;
}

cyclical_buffer::~cyclical_buffer() {
    delete[] _data;
    delete[] _occupied;
}

cyclical_buffer::side cyclical_buffer::sideof(const size_t idx) const {
    if (_tail <= _head) {
        if (_tail <= idx && idx <= _head)
            return LEFT;
    } else {
        if (idx <= _head)
            return LEFT;
        else if (_tail <= idx && idx < rounded_cap())
            return RIGHT;
    }
    return NONE;
}

void cyclical_buffer::reset(const size_t psize) {
    _tail  = _head = 0;
    _psize = psize;
    memset(_data, 0, _capacity);
    memset(_occupied, 0, _capacity);
}

void cyclical_buffer::pop_tail(uint8_t* dst, const size_t nbytes) {
    ENSURE(nbytes % _psize == 0);
    size_t fst_chunk = nbytes;
    if (_tail <= _head)
        ENSURE(sideof(_tail + nbytes - 1) == LEFT);
    else if (fst_chunk > rounded_cap() - _tail) {
        fst_chunk = rounded_cap() - _tail;
        size_t snd_chunk = nbytes - fst_chunk;
        ENSURE(sideof(snd_chunk) == LEFT);
        memset(_data, 0, snd_chunk);
        memset(_occupied, 0, snd_chunk);
    }

    memcpy(dst, _data + _tail, fst_chunk);
    memset(_data + _tail, 0, fst_chunk);
    memset(_occupied + _tail, 0, fst_chunk);

    _tail = (_tail + nbytes) % rounded_cap();
}

void cyclical_buffer::pop_tail(uint8_t* dst) {
    pop_tail(dst, _psize);
}

void cyclical_buffer::fill_gap(const uint8_t* src, const size_t offset) {
    ENSURE(offset % _psize == 0);
    auto side = sideof(offset);
    ENSURE(side != NONE);
    if (_tail <= _head) {
        memcpy(_data + _tail + offset, src, _psize);
        _occupied[_tail + offset] = true;
    } else {
        size_t pos = _tail + offset;
        if (side == LEFT) 
            pos -= rounded_cap() - _tail;
        memcpy(_data + pos, src, _psize);
        _occupied[pos] = true;
    }
}

void cyclical_buffer::push_head(const uint8_t* src, const size_t offset) {
    ENSURE(offset % _psize == 0);
    if (offset >= rounded_cap()) {
        reset(_psize);
        memcpy(_data, src, _psize);
        _occupied[0] = true;
        _head        = _psize;
        _tail        = (_head + _psize) % rounded_cap();
        return;
    }

    size_t write_pos = (_head + offset) % rounded_cap();

    if (write_pos >= _head) {
        memset(_data + _head, 0, write_pos - _head);
        memset(_occupied + _head, 0, write_pos - _head);
    } else {
        memset(_data + _head, 0, rounded_cap() - _head);
        memset(_data, 0, write_pos);
        memset(_occupied + _head, 0, rounded_cap() - _head);
        memset(_occupied, 0, write_pos);
    }
    memcpy(_data + write_pos, src, _psize);
    _occupied[write_pos] = true;

    size_t virt_new_head = _head + offset + _psize; 
    size_t new_head      = virt_new_head % rounded_cap();
    size_t new_tail      = (new_head + _psize) % rounded_cap();
    if (virt_new_head >= rounded_cap() && ((_tail <= _head && _tail <= new_head) || _head < _tail))
        _tail = new_tail;
    else if (_head < _tail && _tail <= new_head)
        _tail = new_tail;
    _head = new_head;
}

size_t cyclical_buffer::range() const {
    if (_tail <= _head)
        return _head - _tail;
    else 
        return _head + (rounded_cap() - _tail);
}

bool cyclical_buffer::empty() const {
    return range() == 0;
}


size_t cyclical_buffer::psize() const {
    return _psize;
}

size_t cyclical_buffer::tail() const {
    return _tail;
}

size_t cyclical_buffer::head() const {
    return _head;
}

bool cyclical_buffer::occupied(const size_t idx) const {
    return _occupied[idx];
}
