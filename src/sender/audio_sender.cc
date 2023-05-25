#include "audio_sender.hh"

#include "../common/err.hh"

#include <unistd.h>
#include <poll.h>

#define MAIN        1
#define NUM_POLLFDS 2

AudioSenderWorker::AudioSenderWorker(
    const volatile sig_atomic_t& running, 
    const sockaddr_in& data_addr,
    const size_t psize,
    const uint64_t session_id,
    const SyncedPtr<CircularBuffer>& packet_cache,
    const int main_fd
) 
    : Worker(running) 
    , _packet_cache(packet_cache)
    , _psize(psize)
    , _session_id(session_id)
    , _main_fd(main_fd)
{
    _data_socket.set_broadcast();
    _data_socket.connect(data_addr);
}

AudioSenderWorker::~AudioSenderWorker() {
    if (-1 == close(_main_fd))
        fatal("close");
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
    poll_fds[STDIN_FILENO].fd = STDIN_FILENO;
    poll_fds[MAIN].fd         = _main_fd;
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    size_t first_byte_num = 0;
    while (running) {
        int poll_status = poll(poll_fds, NUM_POLLFDS, -1);
        if (poll_status == -1)
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
        if (poll_fds[MAIN].revents & POLLIN) {
            poll_fds[MAIN].revents = 0;
            break; // we've been signalled to finish
        }
    }
}
