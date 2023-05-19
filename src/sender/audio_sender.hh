#pragma once

#include "../common/worker.hh"
#include "../common/datagram.hh"
#include "../common/udp_socket.hh"
#include "../common/ptr.hh"
#include "../common/circular_buffer.hh"

#include <netinet/in.h>

struct AudioSenderWorker : public Worker {
private:
    UdpSocket _data_socket;
    SyncedPtr<CircularBuffer> _packet_cache;
    size_t _psize;
    uint64_t _session_id;
    int _main_fd;

    void send_packet(const AudioPacket& packet);
public:
    AudioSenderWorker() = delete;
    AudioSenderWorker(
        const volatile sig_atomic_t& running, 
        const sockaddr_in& mcast_addr,
        const size_t psize,
        const uint64_t session_id,
        const SyncedPtr<CircularBuffer>& packet_cache,
        const int main_fd
    );
    ~AudioSenderWorker();
    
    void run() override;
};
