#include "udp_socket.hh"

#include "log.hh"

#include <unistd.h>

UdpSocket::UdpSocket() : _mcast_recv_enabled(false) {
    if ((_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        fatal("socket");
    set_local_port(0);
}

UdpSocket::~UdpSocket() {
    if (_fd != -1) {
        if (_mcast_recv_enabled)
            disable_mcast_recv();
        if (close(_fd))
            fatal("close");
    }
    _fd = -1;
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) {
    _fd                 = other._fd;
    other._fd           = -1;
    _mcast_recv_enabled = other._mcast_recv_enabled;
    _ipmreq             = other._ipmreq;
    local_addr          = other.local_addr;
    conn_addr           = other.conn_addr;
    return *this;
}

int UdpSocket::fd() const {
    return _fd;
}

void UdpSocket::set_local_port(const in_port_t port) {
    local_addr.sin_family      = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port        = htons(port);
}

void UdpSocket::set_opt(const int proto, const int type, void* val, const size_t val_size) {
    if (setsockopt(_fd, proto, type, val, val_size) == -1)
        fatal("setsockopt");
}

void UdpSocket::set_broadcast() {
    int val = 1;
    set_opt(SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));
}

void UdpSocket::set_reuseaddr() {
    int val = 1;
    set_opt(SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
}

void UdpSocket::set_mcast_ttl(int ttl) {
    set_opt(SOL_SOCKET, SO_REUSEADDR, &ttl, sizeof(ttl));
}

void UdpSocket::set_drop_membership() {
    set_opt(IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, &_ipmreq, sizeof(_ipmreq));
}

void UdpSocket::set_sending_timeout(const int secs) {
    struct timeval timeout;
    timeout.tv_sec  = secs;
    timeout.tv_usec = 0;
    set_opt(SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

void UdpSocket::set_receiving_timeout(const int secs) {
    struct timeval timeout;
    timeout.tv_sec  = secs;
    timeout.tv_usec = 0;
    set_opt(SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void UdpSocket::enable_mcast_recv(const sockaddr_in& multicast_addr, const sockaddr_in& source_addr) {
    _ipmreq.imr_interface.s_addr = INADDR_ANY;
    _ipmreq.imr_multiaddr        = multicast_addr.sin_addr;
    _ipmreq.imr_sourceaddr       = source_addr.sin_addr;
    _mcast_recv_enabled          = true;
    set_opt(IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, &_ipmreq, sizeof(_ipmreq));
}

void UdpSocket::disable_mcast_recv() {
    _mcast_recv_enabled = false;
    set_drop_membership();
}

void UdpSocket::connect(const sockaddr_in& conn_addr) {
    this->conn_addr = conn_addr;
    if (::connect(_fd, (sockaddr*)&conn_addr, sizeof(conn_addr)) == -1)
        fatal("connect");
}

void UdpSocket::bind(const in_port_t port) {
    set_local_port(port);
    if (::bind(_fd, (sockaddr*)&local_addr, sizeof(local_addr)) == -1)
        fatal("bind");
}

ssize_t UdpSocket::sendto(const void* buf, const size_t nbytes, const sockaddr_in& dst_addr) const {
    return ::sendto(_fd, buf, nbytes, 0, (sockaddr*)&dst_addr, sizeof(dst_addr));
}

ssize_t UdpSocket::recvfrom(void* buf, const size_t nbytes, sockaddr_in& src_addr) const {
    socklen_t addr_len = sizeof(src_addr);
    return ::recvfrom(_fd, buf, nbytes, 0, (sockaddr*)&src_addr, &addr_len);
}

size_t UdpSocket::read(void* buf, const size_t nbytes) const {
    return ::read(_fd, buf, nbytes);
}
