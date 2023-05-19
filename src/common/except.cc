#include "except.hh"

const char* RadioException::what() const noexcept {
    return msg.c_str();
}
