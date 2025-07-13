#pragma once

#include <string>
#include <exception>

struct RadioException : public std::exception {
private:
    std::string msg;
public:
    RadioException(const std::string& msg) : msg(std::move(msg)) {}

    const char* what() const noexcept override {
        return msg.c_str();
    }
};
