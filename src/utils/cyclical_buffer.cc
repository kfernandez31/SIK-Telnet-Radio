#include "./cyclical_buffer.hh"
#include "./err.hh"

#include <unistd.h>
#include <cstring>

#include <algorithm>

cyclical_buffer::cyclical_buffer(const size_t capacity) 
    : capacity(capacity), tail(0), head(0), psize(0)
{
    data      = new uint8_t[capacity]();
    populated = new bool[capacity]();
}

size_t cyclical_buffer::rounded_cap() const {
    return capacity / psize * psize;
}

cyclical_buffer::~cyclical_buffer() {
    delete[] data;
    delete[] populated;
}

cyclical_buffer::side cyclical_buffer::sideof(const size_t idx) const {
    if (tail <= head) {
        if (tail <= idx && idx <= head)
            return LEFT;
    } else {
        if (idx <= head)
            return LEFT;
        else if (tail <= idx && idx < rounded_cap())
            return RIGHT;
    }
    return NONE;
}

void cyclical_buffer::reset(const size_t psize) {
    tail = head = 0;
    this->psize = psize;
    memset(data, 0, capacity);
    memset(populated, 0, capacity);
}

void cyclical_buffer::dump(const size_t nbytes) {
    ENSURE(nbytes % psize == 0);
    if (tail <= head) {
        ENSURE(sideof(tail + nbytes - 1) == LEFT);

        ssize_t nwritten = write(STDOUT_FILENO, data + tail, nbytes);
        VERIFY(nwritten);
        ENSURE((size_t)nwritten == nbytes);

        memset(data + tail, 0, nbytes);
        memset(populated + tail, 0, nbytes);
    } else {
        size_t fst_chunk = std::max(nbytes, rounded_cap() - tail);
        size_t snd_chunk = nbytes - fst_chunk;
        ENSURE(sideof(snd_chunk) == LEFT);

        ssize_t nwritten = write(STDOUT_FILENO, data + tail, fst_chunk);
        VERIFY(nwritten);
        ENSURE((ssize_t)fst_chunk == nwritten);

        nwritten = write(STDOUT_FILENO, data, snd_chunk);    
        VERIFY(nwritten);
        ENSURE((ssize_t)snd_chunk == nwritten);

        memset(data + tail, 0, fst_chunk);
        memset(data, 0, snd_chunk);
        memset(populated + tail, 0, fst_chunk);
        memset(populated, 0, snd_chunk);
    }
    tail = (tail + nbytes) % rounded_cap();
}

void cyclical_buffer::fill_gap(const uint8_t* src, const size_t offset) {
    ENSURE(offset % psize == 0);
    auto side = sideof(offset);
    ENSURE(side != NONE);
    if (tail <= head) {
        memcpy(data + tail + offset, src, psize);
        populated[tail + offset] = true;
    } else {
        if (side == RIGHT) { //TODO: może skrócić
            memcpy(data + tail + offset, src, psize);
            populated[tail + offset] = true;
        } else {
            size_t pos = offset - rounded_cap() - tail;
            memcpy(data + pos, src, psize);
            populated[pos] = true;
        }
    }
}

void cyclical_buffer::advance(const uint8_t* src, const size_t offset) {
    ENSURE(offset % psize == 0);
    if (offset >= rounded_cap()) {
        reset(psize);
        memcpy(data, src, psize);
        populated[0] = true;
        head = psize;
        tail = (head + psize) % rounded_cap();
        return;
    }

    size_t write_pos = (head + offset) % rounded_cap();

    if (write_pos >= head) {
        memset(data + head, 0, write_pos - head);
        memset(populated + head, 0, write_pos - head);
    } else {
        memset(data + head, 0, rounded_cap() - head);
        memset(data, 0, write_pos);
        memset(populated + head, 0, rounded_cap() - head);
        memset(populated, 0, write_pos);
    }
    memcpy(data + write_pos, src, psize);
    populated[write_pos] = true;

    size_t virt_new_head = head + offset + psize; 
    size_t new_head      = virt_new_head % rounded_cap();
    size_t new_tail      = (new_head + psize) % rounded_cap();
    //eprintln("    advance --- virt_new_head = %zu, new_head = %zu, new_tail = %zu", virt_new_head, new_head, new_tail);
    if (virt_new_head >= rounded_cap()) { // fold
        if ((tail <= head && tail <= new_head) || head < tail) {
            //eprintln("    advance --- case 1");
            tail = new_tail;
        }
    } else if (head < tail && tail <= new_head) {
        //eprintln("    advance --- case 1");
        tail = new_tail;
    }
    head = new_head;
}

size_t cyclical_buffer::range() const {
    if (tail <= head)
        return head - tail;
    else 
        return head + (rounded_cap() - tail);
}