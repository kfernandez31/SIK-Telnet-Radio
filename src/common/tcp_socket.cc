#include "tcp_socket.hh"

#include "err.hh"
#include "except.hh"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>

TcpSocket::TcpSocket(const in_port_t port, const size_t buf_size, const size_t queue_len) 
    : _conn_fd(-1), _buf_size(buf_size), _queue_len(queue_len), _in(*this), _out(*this)
{
    if (-1 == (_fd = socket(PF_INET, SOCK_STREAM, 0)))
        fatal("socket");

    int optval = 1;
    if (-1 == setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
        fatal("setsockopt");

    _local_addr.sin_family      = AF_INET;
    _local_addr.sin_addr.s_addr = INADDR_ANY;
    _local_addr.sin_port        = htons(port);

    if (-1 == bind(_fd, (sockaddr *) &_local_addr, sizeof(_local_addr)))
        fatal("bind");

    _buf = std::make_unique<char[]>(buf_size + 1);
}

TcpSocket::~TcpSocket() {
    if (_conn_fd != -1) {
        if (-1 == shutdown(_conn_fd, SHUT_RDWR))
            fatal("shutdown");
        if (-1 == ::close(_conn_fd))
            fatal("close");
    }
    if (_fd != -1) {
        if (-1 == shutdown(_fd, SHUT_RDWR))
            fatal("shutdown");
        if (-1 == ::close(_fd))
            fatal("close");
    }
}

void TcpSocket::listen() {
    if (-1 == ::listen(_fd, _queue_len))
        fatal("listen");
}

void TcpSocket::accept() {
    socklen_t _conn_addr_len = sizeof(_conn_addr);
    if (-1 == (_conn_fd = ::accept(_fd, (sockaddr*)&_conn_addr, &_conn_addr_len)))
        fatal("accept");
}

TcpSocket::OutStream& TcpSocket::out() {
    return _out;
}

TcpSocket::InStream& TcpSocket::in() {
    return _in;
}

//----------------------------InStream------------------------------------

TcpSocket::InStream::InStream(const TcpSocket& socket) : socket(socket) {}

bool TcpSocket::InStream::read() {
    ssize_t nread = ::read(socket._conn_fd, socket._buf.get(), socket._buf_size);
    if (nread == -1)
        fatal("read");
    if (nread == 0)
        return false;
    socket._buf[nread] = '\0';
    ss.str("");
    ss.clear();
    ss << socket._buf.get();
    return true;
}

TcpSocket::InStream& TcpSocket::InStream::getline(std::string& _buf) {
  if (ss.eof())
    _buf.clear();
  else {
    std::getline(ss, _buf);
    ss.get();
  }
  return *this;
}

bool TcpSocket::InStream::eof() {
    return ss.eof() && !read();
}

//----------------------------OutStream------------------------------------

TcpSocket::OutStream::OutStream(const TcpSocket& socket) : socket(socket) {
    ss.str("");
}

TcpSocket::OutStream& TcpSocket::OutStream::operator<<(OutStream& (*f)(OutStream&)) {
    f(*this);
    return *this;
}

void TcpSocket::OutStream::write() {
    const std::string& str = ss.str();
    if (str.length() != ::write(socket._conn_fd, str.c_str(), str.length()))
        throw RadioException("write to socket failed");
    ss.str("");
    ss.clear();
}

TcpSocket::OutStream& TcpSocket::OutStream::flush(OutStream& stream) {
    stream.write();
    return stream;
}

TcpSocket::OutStream& TcpSocket::OutStream::endl(OutStream& stream) {
    return stream << '\n' << OutStream::flush;
}
