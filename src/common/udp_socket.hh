#pragma once

#include "net.hh"
#include <netinet/in.h>

struct UdpSocket {
private:
    int _fd;
    bool _mcast_recv_enabled, _connected;
    ip_mreq_source _ipmreq;
    void set_opt(const int proto, const int type, void* val, const size_t val_size);
    void set_local_port(const in_port_t port);
public:
    sockaddr_in local_addr, conn_addr;

    UdpSocket();
    ~UdpSocket();

    int fd() const;

    UdpSocket& operator=(UdpSocket&& other);

    void set_broadcast(); 
    void set_receiving_timeout(const int secs);
    void set_reuseaddr();
    void set_mcast_ttl(int ttl = DEFAULT_TTL);
    void set_sending_timeout(const int secs);
    void set_add_membership();
    void set_drop_membership();

    void enable_mcast_recv(const sockaddr_in& multicast_addr, const sockaddr_in& source_addr);
    void disable_mcast_recv(); 

    void bind(const in_port_t port = 0);
    void connect(const sockaddr_in& conn_addr);

    void write(const void* buf, const size_t nbytes) const;
    size_t read(void* buf, const size_t nbytes) const;
    ssize_t sendto(const void* buf, const size_t nbytes, const sockaddr_in &dst_addr) const;
    ssize_t recvfrom(void* buf, const size_t nbytes, sockaddr_in& src_addr) const;

    static const int DEFAULT_TTL = 4;
};
