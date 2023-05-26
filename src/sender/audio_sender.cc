#include "audio_sender.hh"

#include "../common/err.hh"

#include <unistd.h>
#include <poll.h>

#define INTERNAL_EVENT 0        
#define NUM_POLLFDS    2

AudioSenderWorker::AudioSenderWorker(
    const volatile sig_atomic_t& running, 
    const sockaddr_in& data_addr,
    const SyncedPtr<CircularBuffer>& packet_cache,
    const SyncedPtr<EventPipe>& current_event,
    const size_t psize,
    const uint64_t session_id
) 
    : Worker(running) 
    , _packet_cache(packet_cache)
    , _current_event(current_event)
    , _psize(psize)
    , _session_id(session_id)
{
    _data_socket.connect(data_addr);
}

void AudioSenderWorker::send_packet(const AudioPacket& packet) {
    _data_socket.write(packet.bytes.data(), packet.total_size());
    auto lock = _packet_cache.lock();
    _packet_cache->try_put(packet);
}

void AudioSenderWorker::run() {
    char audio_buf[_psize + 1];
    size_t nread = 0;
    memset(audio_buf, 0, sizeof(audio_buf));

    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[STDIN_FILENO].fd   = STDIN_FILENO;
    poll_fds[INTERNAL_EVENT].fd = _current_event->in_fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    size_t first_byte_num = 0;
    while (running) {
        if (-1 == poll(poll_fds, NUM_POLLFDS, -1))
            fatal("poll");

        if (poll_fds[STDIN_FILENO].revents & POLLIN) {
            ssize_t res = read(STDIN_FILENO, audio_buf + nread, _psize - nread);
            poll_fds[STDIN_FILENO].revents = 0;
            if (res == -1)
                fatal("read");
            if (res == 0)
                break; // end of input or incomplete packet
            nread += res;
            if (nread == _psize) {
                send_packet(std::move(AudioPacket(first_byte_num, _session_id, audio_buf, _psize)));
                first_byte_num += _psize;
                memset(audio_buf, 0, sizeof(audio_buf));
                nread = 0;
            }
        } 

        if (poll_fds[INTERNAL_EVENT].revents & POLLIN) {
            poll_fds[INTERNAL_EVENT].revents = 0;
            auto event = _current_event->get_event();
            if (event == EventPipe::EventType::SIG_INT && !running)
                break;
        }
    }
}
