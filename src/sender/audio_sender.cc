#include "audio_sender.hh"

#include <unistd.h>
#include <poll.h>

#include <thread>
#include <chrono>

#define MY_EVENT    1      
#define NUM_POLLFDS 2

AudioSenderWorker::AudioSenderWorker(
    const volatile sig_atomic_t& running, 
    const sockaddr_in& mcast_addr,
    const SyncedPtr<CircularBuffer>& packet_cache,
    const SyncedPtr<EventQueue>& my_event,
    const size_t psize,
    const uint64_t session_id
) 
    : Worker(running, "AudioSender") 
    , _packet_cache(packet_cache)
    , _my_event(my_event)
    , _psize(psize)
    , _session_id(session_id)
    , _mcast_addr(mcast_addr)
{
    _data_socket.set_mcast_ttl();
    //TODO: ?
    // _data_socket.set_reuseaddr();
    // _data_socket.set_reuseport();
}

void AudioSenderWorker::send_packet(AudioPacket&& packet) {
    log_debug("[%s] Sending packet %llu of size %zu and session %llu", 
        name.c_str(), packet.first_byte_num, packet.psize, packet.session_id);

    _data_socket.sendto(packet.bytes.get(), TOTAL_PSIZE(packet.psize), _mcast_addr);

    auto lock = _packet_cache.lock();
    _packet_cache->try_put(packet);
}

void AudioSenderWorker::run() {
    char audio_buf[_psize + 1];
    size_t nread = 0;
    memset(audio_buf, 0, sizeof(audio_buf));

    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[STDIN_FILENO].fd   = STDIN_FILENO;
    poll_fds[MY_EVENT].fd = _my_event->in_fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    size_t first_byte_num = 0;
    while (running) {
        if (-1 == poll(poll_fds, NUM_POLLFDS, -1))
            fatal("poll");
            
        if (poll_fds[MY_EVENT].revents & POLLIN) {
            poll_fds[MY_EVENT].revents = 0;
            EventQueue::EventType event_val = _my_event->pop();
            switch (event_val) {
                case EventQueue::EventType::TERMINATE:
                    return;
                default: break;
            }
        }

        if (poll_fds[STDIN_FILENO].revents & POLLIN) {
            poll_fds[STDIN_FILENO].revents = 0;
            ssize_t res = read(STDIN_FILENO, audio_buf + nread, _psize - nread);
            log_debug("[%s] read %zu bytes", name.c_str(), res);
            if (res == -1)
                fatal("read");
            if (res == 0)
                break; // end of input or incomplete packet
            nread += res;
            if (nread == _psize) {
                log_info("[%s] sending packet #%llu", name.c_str(), first_byte_num);
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                send_packet(AudioPacket(_session_id, first_byte_num, audio_buf, _psize));
                first_byte_num += _psize;
                memset(audio_buf, 0, sizeof(audio_buf));
                nread = 0;
            }
        } 
    }
}
