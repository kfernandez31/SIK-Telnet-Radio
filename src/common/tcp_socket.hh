#pragma once

#include "err.h"

#include <netinet/in.h>

#include <memory>
#include <sstream>
#include <string>

struct TcpClientSocket {
public:
    TcpClientSocket(const int fd);
    ~TcpClientSocket();

    TcpClientSocket& operator=(TcpClientSocket&& other);
    
    int fd() const;

    bool read(void* buf, const size_t nbytes);
    void write(const std::string& msg);
private:
    int _fd;
};

struct TcpServerSocket {
public:
    TcpServerSocket() = delete;
    TcpServerSocket(const in_port_t port, const size_t queue_len = DEFAULT_QUEUE_LEN);
    ~TcpServerSocket();

    void             listen();
    TcpClientSocket  accept();
    in_port_t port() const;
    int       fd() const;
private:
    static const size_t DEFAULT_QUEUE_LEN = 42;
    int _fd;
    in_port_t _port;
    size_t _queue_len;
};
