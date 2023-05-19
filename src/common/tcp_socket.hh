#pragma once

#include "err.h"

#include <netinet/in.h>

#include <memory>
#include <sstream>

struct TcpSocket {
public:
    TcpSocket(const in_port_t port, const size_t buf_size, const size_t queue_len);
    ~TcpSocket();

    void listen();
    void accept();

    struct InStream {
        std::stringstream ss;
        const TcpSocket& socket;
        bool read();
    public:
        InStream() = delete;
        InStream(const TcpSocket& socket);

        template<typename T>
        InStream& operator>>(const T& val);

        InStream& getline(std::string& buf);
        bool eof();
    };

    struct OutStream {
    private:
        std::stringstream ss;
        const TcpSocket& socket;
    public:
        OutStream() = delete;
        OutStream(const TcpSocket& socket);

        template<typename T>
        OutStream& operator<<(const T& val);

        OutStream& operator<<(OutStream& (*f)(OutStream&));

        void write();
        static OutStream& flush(OutStream& stream);
        static OutStream& endl(OutStream& stream);
    };

    OutStream& out();
    InStream& in();
private:
    int _fd, _conn_fd;
    size_t _buf_size, _queue_len;
    sockaddr_in _local_addr, _conn_addr;
    std::unique_ptr<char[]> _buf;
    InStream _in;
    OutStream _out;
};
