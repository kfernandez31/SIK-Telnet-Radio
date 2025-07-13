#include "tcp_socket.hh"

#include "log.hh"
#include "except.hh"

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>


//----------------------------TcpServerSocket------------------------------------

TcpServerSocket::TcpServerSocket(const in_port_t port, const size_t queue_len)
    : _port(port)
    , _queue_len(queue_len)
{
    if ((_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        fatal("socket");

    sockaddr_in server_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(_port);
    if (bind(_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        fatal("bind");
}

TcpServerSocket::~TcpServerSocket() {
    if (::close(_fd))
        fatal("close");
}

void TcpServerSocket::listen() {
    if (::listen(_fd, _queue_len) == -1)
        fatal("listen");
}

TcpClientSocket TcpServerSocket::accept() {
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd = ::accept(_fd, (sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1)
        throw RadioException("Accepting a new connection failed");
    return TcpClientSocket(client_fd);
}

int TcpServerSocket::fd() const {
    return _fd;
}

in_port_t TcpServerSocket::port() const {
    return _port;
}

//----------------------------TcpClientSocket------------------------------------

TcpClientSocket::TcpClientSocket(const int fd)
    : _fd(fd) {}

TcpClientSocket::~TcpClientSocket() {
    if (_fd != -1 && ::close(_fd) == -1)
        log_error("Failed to close socket");
}

TcpClientSocket& TcpClientSocket::operator=(TcpClientSocket&& other) {
    _fd       = other._fd;
    other._fd = -1;
    return *this;
}

int TcpClientSocket::fd() const {
    return _fd;
}

bool TcpClientSocket::read(void* buf, const size_t nbytes) {
    ssize_t nread = ::read(_fd, buf, nbytes);
    if (nread == -1)
        throw RadioException("Reading from socket failed");
    return nread > 0;
}

void TcpClientSocket::write(const std::string& msg) {
    if (::write(_fd, msg.c_str(), msg.size()) == -1)
        throw RadioException("Failed to write to socket");
}
