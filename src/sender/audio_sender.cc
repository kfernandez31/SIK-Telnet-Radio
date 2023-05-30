#include "audio_sender.hh"

#include <unistd.h>
#include <poll.h>

#include <thread>
#include <chrono>

#define MY_EVENT    1      
#define NUM_POLLFDS 2

//TODO:
// Credits: Hubert Lubański
// static bool simulate_interference() {
//     static const int max_lost_per_burst = 8;
//     static const int min_lost_per_burst = 1;
//     static const int min_without_loss   = 400;
//     static const int interference_noise = 50;
//     static int counter = 0, p = max_lost_per_burst;
//     static int T = min_without_loss + interference_noise;

//     if (++counter + p != T)
//         return false;
//     if (p != 0)
//         p--;
//     else {
//         p = min_lost_per_burst + rand() % (max_lost_per_burst - min_lost_per_burst);
//         counter = 0;
//         T = min_without_loss + rand() % interference_noise;
//     }
//     return true;
// }

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
}

void AudioSenderWorker::send_packet(AudioPacket&& packet) {
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
            if (res == -1)
                fatal("read");
            if (res == 0)
                break; // end of input or incomplete packet
            nread += res;
            if (nread == _psize) {
                log_info("[%s] sending packet #%llu", name.c_str(), first_byte_num);
                send_packet(AudioPacket(_session_id, first_byte_num, audio_buf, _psize));
                first_byte_num += _psize;
                memset(audio_buf, 0, sizeof(audio_buf));
                nread = 0;
            }
        } 
    }
}
