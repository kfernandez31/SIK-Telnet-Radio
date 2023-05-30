#pragma once

#include "../common/event_queue.hh"
#include "../common/worker.hh"
#include "../common/datagram.hh"
#include "../common/udp_socket.hh"
#include "../common/synced_ptr.hh"
#include "../common/circular_buffer.hh"

#include <netinet/in.h>

struct AudioSenderWorker : public Worker {
private:
    UdpSocket _data_socket;
    SyncedPtr<CircularBuffer> _packet_cache;
    SyncedPtr<EventQueue> _my_event;
    size_t _psize;
    uint64_t _session_id;
    sockaddr_in _mcast_addr;

    void send_packet(AudioPacket&& packet);
public:
    AudioSenderWorker() = delete;
    AudioSenderWorker(
        const volatile sig_atomic_t& running, 
        const sockaddr_in& mcast_addr,
        const SyncedPtr<CircularBuffer>& packet_cache,
        const SyncedPtr<EventQueue>& my_event,
        const size_t psize,
        const uint64_t session_id
    );
    
    void run() override;
};
