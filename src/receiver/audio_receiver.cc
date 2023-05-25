#include "audio_receiver.hh"

#include "../common/err.hh"
#include "../common/net.hh"
#include "../common/except.hh"
#include "../common/datagram.hh"

#include <poll.h>
#include <unistd.h>

#define NO_SESSION  0

#define NETWORK     0
#define MENU        1
#define PRINTER     2
#define LOOKUP      3

AudioReceiverWorker::AudioReceiverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const std::shared_ptr<AudioPrinterWorker>& printer,
    const int printer_fd,
    const int menu_fd,
    const int lookup_fd
)
    : Worker(running)
    , _buffer(buffer)
    , _stations(stations)
    , _current_station(current_station)
    , _printer(printer)
{
    _poll_fds[MENU].fd    = menu_fd;
    _poll_fds[PRINTER].fd = printer_fd;
    _poll_fds[LOOKUP].fd  = lookup_fd;
    //TODO: jakieś ustawienia socketa
    _poll_fds[NETWORK].fd = _data_socket.fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        _poll_fds[i].events  = POLLIN;
        _poll_fds[i].revents = 0;
    }
}

AudioReceiverWorker::~AudioReceiverWorker() {
    if (-1 == close(_poll_fds[NETWORK].fd))
        fatal("close");
    if (-1 == close(_poll_fds[PRINTER].fd))
        fatal("close");
}

AudioPacket AudioReceiverWorker::read_packet() {
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();

    char pkt_buf[MTU + 1] = {0};
    size_t nread = _data_socket.recvfrom(pkt_buf, MTU, (*_current_station)->data_addr); //TODO: chyba trzeba zconnectować żeby ten adres się nie zmienił :|
    if (-1 == nread)
        fatal("recvfrom");
    if ((size_t)nread <= 2 * sizeof(uint64_t))
        throw RadioException("Malformed packet");
    return AudioPacket(pkt_buf);
}

void AudioReceiverWorker::run() {
    bool has_printed = false;
    uint64_t cur_session = NO_SESSION;

    for (;;) {
        // this doesn't need a timeout because of the remover
        int poll_status = poll(_poll_fds, NUM_POLLFDS, -1); 
        if (poll_status == -1)
            fatal("poll");

        if (_poll_fds[MENU].revents & POLLIN) {
            _poll_fds[MENU].revents = 0;
            if (!running) // we have been signalled to stop
                break;
            // if not, switch to the new station and reset variables
            _poll_fds[MENU].fd = (*_current_station)->data_socket.fd();
            cur_session = NO_SESSION;
        }
        if (_poll_fds[LOOKUP].revents & POLLIN) {
            _poll_fds[LOOKUP].revents = 0;
            if (!running) // we have been signalled to stop
                break;
            // if not, switch to the new station and reset variables
            _poll_fds[LOOKUP].fd = (*_current_station)->data_socket.fd();
            cur_session = NO_SESSION;
        }
        // packet loss detected
        if (_poll_fds[PRINTER].revents & POLLIN) {
            cur_session = NO_SESSION;
            _poll_fds[PRINTER].revents = 0;
        }
        // got a new packet
        if (_poll_fds[NETWORK].revents & POLLIN) {
            try {
                AudioPacket packet = read_packet();
                _poll_fds[NETWORK].revents = 0;

                if (packet.session_id < cur_session) {
                    logerr("Ignoring old session %llu...", packet.session_id);
                    continue;
                }

                if (_buffer->psize() > _buffer->capacity()) {
                    logerr("Packet size too large, ignoring session %llu...", packet.session_id);
                    cur_session = NO_SESSION;
                    continue;
                }

                if (packet.session_id > cur_session) {
                    logerr("New session %llu!", packet.session_id);
                    cur_session = packet.session_id;
                    has_printed = false;
                    auto lock   = _buffer.lock();
                    _buffer->reset(packet.psize(), packet.first_byte_num);
                }

                if (packet.first_byte_num < _buffer->byte0()) {
                    logerr("Packet %zu of session %llu arrived too late, ignoring...", 
                        packet.first_byte_num, packet.session_id);
                    continue;
                }
                
                auto lock = _buffer.lock();
                _buffer->try_put(packet);
                if (has_printed || (!has_printed && (packet.first_byte_num + _buffer->psize() - 1) 
                    >= (_buffer->byte0() + _buffer->capacity() / 4 * 3))) 
                {
                    has_printed = true;
                    _printer->signal();
                }
            } catch (std::exception& e) {
                logerr("Failed to read packet: ", e.what());
            }
        }
    }
}
