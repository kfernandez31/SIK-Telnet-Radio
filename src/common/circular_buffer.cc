#include "./circular_buffer.hh"
#include "./log.hh"

#include <unistd.h>
#include <cstring>
#include <cassert>

#include <algorithm>

CircularBuffer::CircularBuffer(const size_t capacity) 
    : _capacity(capacity)
    , _psize(0)
    , _tail(0)
    , _head(0)
    , _abs_head(0)
    , _byte0(0)
{
    try {
        _data     = new char[capacity]();
        _occupied = new bool[capacity]();
    } catch (const std::exception& e) {
        fatal(e.what());
    }
}

size_t CircularBuffer::capacity() const {
    return _capacity;
}

size_t CircularBuffer::rounded_cap() const {
    return _capacity / _psize * _psize;
}

CircularBuffer::~CircularBuffer() {
    delete[] _data;
    delete[] _occupied;
}

CircularBuffer::side CircularBuffer::sideof(const size_t idx) const {
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

void CircularBuffer::reset(const size_t psize) {
    _tail  = _head = 0;
    _psize = psize;
    memset(_data, 0, _capacity);
    memset(_occupied, 0, _capacity);
}

void CircularBuffer::reset(const size_t psize, const uint64_t abs_head_) {
    _abs_head = _byte0 = abs_head_;
    reset(psize);
}

void CircularBuffer::dump_tail(const size_t nbytes) {
    assert(nbytes % _psize == 0);
    size_t fst_chunk = nbytes;
    size_t snd_chunk = 0;
    if (_tail > _head && fst_chunk > rounded_cap() - _tail) {
        fst_chunk = rounded_cap() - _tail;
        snd_chunk = nbytes - fst_chunk;
    }

    size_t nwritten = write(STDOUT_FILENO, _data + _tail, fst_chunk);
    nwritten += write(STDOUT_FILENO, _data, snd_chunk);
    if (nwritten != fst_chunk + snd_chunk)
        fatal("write");

    memset(_data + _tail, 0, fst_chunk);
    memset(_occupied + _tail, 0, fst_chunk);

    memset(_data, 0, snd_chunk);
    memset(_occupied, 0, snd_chunk);

    _tail = (_tail + nbytes) % rounded_cap();
}

void CircularBuffer::fill_gap(const AudioPacket& packet) {
    size_t tail_offset = packet.first_byte_num - abs_tail();

    assert(tail_offset % _psize == 0);
    auto side = sideof(tail_offset);
    assert(side != NONE);
    size_t pos = _tail + tail_offset;
    if (_tail > _head && side == LEFT) 
        pos -= rounded_cap() - _tail;
    memcpy(_data + pos, packet.audio_data(), _psize);
    _occupied[pos] = true;
}

void CircularBuffer::try_push_head(const AudioPacket& packet) {
    uint64_t head_offset = packet.first_byte_num - _abs_head;
    assert(head_offset % _psize == 0);
    if (head_offset > _psize) // dismiss, packet is too far ahead
        return;

    _abs_head = packet.first_byte_num + _psize;
    if (head_offset >= rounded_cap()) {
        reset(_psize);
        memcpy(_data, packet.audio_data(), _psize);
        _occupied[0] = true;
        _head        = _psize;
        _tail        = (_head + _psize) % rounded_cap();
    } else {
        size_t write_pos = (_head + head_offset) % rounded_cap();

        if (write_pos >= _head) {
            memset(_data + _head, 0, write_pos - _head);
            memset(_occupied + _head, 0, write_pos - _head);
        } else {
            memset(_data + _head, 0, rounded_cap() - _head);
            memset(_data, 0, write_pos);
            memset(_occupied + _head, 0, rounded_cap() - _head);
            memset(_occupied, 0, write_pos);
        }
        memcpy(_data + write_pos, packet.audio_data(), _psize);
        _occupied[write_pos] = true;

        size_t virt_new_head = _head + head_offset + _psize; 
        size_t new_head      = virt_new_head % rounded_cap();
        size_t new_tail      = (new_head + _psize) % rounded_cap();
        if (virt_new_head >= rounded_cap() && ((_tail <= _head && _tail <= new_head) || _head < _tail)) 
            _tail = new_tail;
        else if (_head < _tail && _tail <= new_head) 
            _tail = new_tail;
        _head = new_head;
    }
}

void CircularBuffer::try_put(const AudioPacket& packet) {
    if (packet.first_byte_num < abs_tail()) // dismiss, packet is too far behind
        return;
    if (packet.first_byte_num >= _abs_head)  // advance 
        try_push_head(packet);
    else // fill in a gap
        fill_gap(packet);
}

size_t CircularBuffer::cnt_upto_gap() const {
    size_t cnt = 0, i = _tail;
    do {
        if (!_occupied[i])
            break;
        cnt++;
        i = (i + _psize) % rounded_cap();
    } while (i != _tail);
    return cnt;
}

size_t CircularBuffer::range() const {
    if (_tail <= _head)
        return _head - _tail;
    else 
        return _head + (rounded_cap() - _tail);
}

size_t CircularBuffer::psize() const {
    return _psize;
}

size_t CircularBuffer::tail() const {
    return _tail;
}

size_t CircularBuffer::head() const {
    return _head;
}

char* CircularBuffer::data() const {
    return _data;
}

bool CircularBuffer::occupied(const size_t idx) const {
    return _occupied[idx];
}

uint64_t CircularBuffer::abs_head() const {
    return _abs_head;
}

uint64_t CircularBuffer::abs_tail() const {
    return _abs_head - range();
}

uint64_t CircularBuffer::byte0() const {
    return _byte0;
}

uint64_t CircularBuffer::print_threshold() const {
    return _byte0 + _capacity / 4 * 3;
}
