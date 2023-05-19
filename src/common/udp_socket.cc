#include "udp_socket.hh"

#include "err.hh"

#include <unistd.h>

#define TTL_VALUE 4

UdpSocket::UdpSocket(): mcast_recv_enabled(false) {
    if (-1 == (_fd = socket(AF_INET, SOCK_DGRAM, 0)))
        fatal("socket");
    set_local_port(0);
}

UdpSocket::~UdpSocket() {
    if (mcast_recv_enabled)
        disable_mcast_recv();
    if (-1 == shutdown(_fd, SHUT_RDWR))
        fatal("shutdown");
    if (-1 == close(_fd))
        fatal("close");
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
    if (-1 == setsockopt(_fd, proto, type, val, val_size))
        fatal("setsockopt");
}

void UdpSocket::set_reuseaddr() {
    int val = 1;
    set_opt(SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
}

void UdpSocket::set_reuseport() {
    int val = 1;
    set_opt(SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
}

void UdpSocket::set_broadcast() {
    int val = 1;
    set_opt(SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));
}

void UdpSocket::set_mcast_ttl() {
    int val = TTL_VALUE;
    set_opt(SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
}

void UdpSocket::set_add_membership() {
    set_opt(IPPROTO_IP, IP_DROP_MEMBERSHIP, &_ipmreq, sizeof(_ipmreq));
}

void UdpSocket::set_drop_membership() {
    set_opt(IPPROTO_IP, IP_DROP_MEMBERSHIP, &_ipmreq, sizeof(_ipmreq));
}

void UdpSocket::enable_mcast_recv(const sockaddr_in& conn_addr) {
    local_addr.sin_port          = conn_addr.sin_port;
    _ipmreq.imr_interface.s_addr = INADDR_ANY;
    _ipmreq.imr_multiaddr        = conn_addr.sin_addr;
    set_add_membership();
    if (-1 == bind(_fd, (sockaddr*)&local_addr, sizeof(local_addr)))
        fatal("bind");
}

void UdpSocket::disable_mcast_recv() {
    set_drop_membership();
}

void UdpSocket::enable_mcast_send(const sockaddr_in& conn_addr) {
    set_add_membership();
    set_mcast_ttl();
    connect(conn_addr);
}

void UdpSocket::connect(const sockaddr_in& conn_addr) {
    this->conn_addr = conn_addr;
    if (-1 == ::connect(_fd, (sockaddr*)&conn_addr, sizeof(conn_addr)))
        fatal("connect");
}

void UdpSocket::bind_local_port(const in_port_t port) {
    set_local_port(port);
    if (-1 == bind(_fd, (sockaddr *)&local_addr, sizeof(local_addr)))
        fatal("bind");
}

void UdpSocket::write(const void* buf, const size_t nbytes) const {
    if (nbytes != ::write(_fd, buf, nbytes))
        fatal("write");
}

void UdpSocket::sendto(const void* buf, const size_t nbytes, const sockaddr_in& dst_addr) const {
    if (nbytes != ::sendto(_fd, buf, nbytes, 0, (sockaddr*)&dst_addr, sizeof(dst_addr)))
        fatal("sendto");
}

size_t UdpSocket::recvfrom(void* buf, const size_t nbytes, sockaddr_in& src_addr) const {
    socklen_t addr_len = sizeof(src_addr);
    size_t nread;
    if (-1 == (nread = ::recvfrom(_fd, buf, nbytes, 0, (sockaddr*)&src_addr, &addr_len)))
        fatal("recvfrom");
    return nread;
}
