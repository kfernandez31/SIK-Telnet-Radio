#pragma once

#include "err.h"

#include <netinet/in.h>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

struct TcpServerSocket {
public:
    TcpServerSocket() = delete;
    TcpServerSocket(const in_port_t port, const size_t queue_len = DEFAULT_QUEUE_LEN);
    ~TcpServerSocket();

    void listen();
    int  accept();
    in_port_t port() const;
    int       fd() const;
private:
    static const size_t DEFAULT_QUEUE_LEN = 42;
    int _fd;
    in_port_t _port;
    size_t _queue_len;
};

// note to self: this could be more elegant if it inherited from std::istream & std::ostream
struct TcpClientSocket {
public:
    TcpClientSocket() = delete;
    TcpClientSocket(const int fd, const size_t buf_size = DEFAULT_BUF_SIZE);
    ~TcpClientSocket();

    struct InStream {
    private:
        std::stringstream ss;
        const TcpClientSocket& socket;
        bool _eof_bit = false;
        void read();
    public:
        InStream() = delete;
        InStream(const TcpClientSocket& socket);

        template<typename T>
        InStream& operator>>(const T& val) {
            if (!eof())
                ss >> val;
            return *this;
        }

        InStream& getline(std::string& str_buf);
        bool eof();
    };

    struct OutStream {
    private:
        std::stringstream ss;
        const TcpClientSocket& socket;
    public:
        OutStream() = delete;
        OutStream(const TcpClientSocket& socket);

        template<typename T>
        OutStream& operator<<(const T& val) {
            ss << val;
            return *this;
        }
        OutStream& operator<<(OutStream& (*f)(OutStream&));

        void write();
        static OutStream& flush(OutStream& stream);
        static OutStream& endl(OutStream& stream);
    };

    InStream&  in();
    OutStream& out();

    int fd() const;
private:
    static const size_t DEFAULT_BUF_SIZE = 1024;
    int _fd;
    InStream _in;
    OutStream _out;
    size_t _buf_size;
    std::unique_ptr<char[]> _buf;
};
