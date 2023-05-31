#include "tcp_socket.hh"

#include "log.hh"
#include "except.hh"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <iostream>

//----------------------------TcpServerSocket------------------------------------

TcpServerSocket::TcpServerSocket(const in_port_t port, const size_t queue_len)
    : _port(port)
    , _queue_len(queue_len)
{
    if (-1 == (_fd = socket(PF_INET, SOCK_STREAM, 0)))
        fatal("socket");

    sockaddr_in server_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(_port);
    if (-1 == bind(_fd, (sockaddr*)&server_addr, sizeof(server_addr)))
        fatal("bind");
}

TcpServerSocket::~TcpServerSocket() {
    if (-1 == ::close(_fd))
        fatal("close");
}

void TcpServerSocket::listen() {
    if (-1 == ::listen(_fd, _queue_len))
        fatal("listen");
}

int TcpServerSocket::accept() {
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    return ::accept(_fd, (sockaddr*)&client_addr, &client_addr_len);
}

int TcpServerSocket::fd() const {
    return _fd;
}

in_port_t TcpServerSocket::port() const {
    return _port;
}

//----------------------------TcpClientSocket------------------------------------

TcpClientSocket::TcpClientSocket(const int fd, const size_t buf_size)
    : _fd(fd), _in(*this), _out(*this)   
{
   _buf = std::make_unique<char[]>(buf_size + 1);
}

TcpClientSocket::~TcpClientSocket() {
    if (-1 == shutdown(_fd, SHUT_RDWR) && errno)
        log_error("shutdown: %s", strerror(errno));
    if (-1 == ::close(_fd) && errno)
        log_error("shutdown: %s", strerror(errno));
}

int TcpClientSocket::fd() const {
    return _fd;
}

TcpClientSocket::InStream& TcpClientSocket::in() {
    return _in;
}

TcpClientSocket::OutStream& TcpClientSocket::out() {
    return _out;
}

//----------------------------InStream------------------------------------

TcpClientSocket::InStream::InStream(const TcpClientSocket& socket)
    : socket(socket) {}

void TcpClientSocket::InStream::read() {
    ssize_t nread = ::read(socket._fd, socket._buf.get(), socket._buf_size);
    if (nread == -1)
        fatal("read");
    if (nread == 0) {
        _eof_bit = true;
    } else {
        socket._buf[nread] = '\0';
        ss.clear();
        ss.str(socket._buf.get());
    }
}

TcpClientSocket::InStream& TcpClientSocket::InStream::getline(std::string& str_buf) {
    if (ss.eof()) {
        log_debug("branch 1");
        str_buf.clear();
    }
    else {
        log_debug("branch 2");
        std::getline(ss, str_buf);
        ss.get(); // set EOF
    }
    return *this;
}

bool TcpClientSocket::InStream::eof() {
    return _eof_bit;
}

//----------------------------OutStream------------------------------------

TcpClientSocket::OutStream::OutStream(const TcpClientSocket& socket)
    : socket(socket) 
{
    ss.str("");
}

TcpClientSocket::OutStream& TcpClientSocket::OutStream::operator<<(OutStream& (*f)(OutStream&)) {
    f(*this);
    return *this;
}

void TcpClientSocket::OutStream::write() {
    const std::string& str = ss.str();
    if ((ssize_t)str.length() != ::write(socket._fd, str.c_str(), str.length()))
        throw RadioException("write to socket failed");
    ss.str("");
    ss.clear();
}

TcpClientSocket::OutStream& TcpClientSocket::OutStream::flush(OutStream& stream) {
    stream.write();
    return stream;
}

TcpClientSocket::OutStream& TcpClientSocket::OutStream::endl(OutStream& stream) {
    return stream << '\n' << OutStream::flush;
}
