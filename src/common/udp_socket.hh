#pragma once

#include <netinet/in.h>

struct UdpSocket {
private:
    int _fd;
    void set_opt(const int proto, const int type, void* val, const size_t val_size);
    void set_local_port(const in_port_t port);
    bool _mcast_recv_enabled, _connected;
    ip_mreq _ipmreq;
public:
    sockaddr_in local_addr, conn_addr;

    UdpSocket();
    ~UdpSocket();

    int fd() const;

    void set_reuseaddr();
    void set_reuseport();
    void set_broadcast(); 
    void set_mcast_ttl();
    void set_add_membership();
    void set_drop_membership();

    void enable_mcast_send(const sockaddr_in& conn_addr);
    void enable_mcast_recv(const sockaddr_in& conn_addr);
    void disable_mcast_recv(); 

    void bind_local_port(const in_port_t port = 0);
    void connect(const sockaddr_in& conn_addr);

    void write(const void* buf, const size_t nbytes) const;
    void sendto(const void* buf, const size_t nbytes, const sockaddr_in &dst_addr) const;
    size_t recvfrom(void* buf, const size_t nbytes, sockaddr_in& src_addr) const;
};
