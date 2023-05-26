#include "audio_receiver.hh"

#include "../common/err.hh"
#include "../common/net.hh"
#include "../common/except.hh"
#include "../common/datagram.hh"

#include <poll.h>
#include <unistd.h>

#define NO_SESSION     0

#define INTERNAL_EVENT 0
#define NETWORK        1
#define NUM_POLLFDS    2

AudioReceiverWorker::AudioReceiverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventPipe>& current_event,
    const std::shared_ptr<AudioPrinterWorker>& printer
)
    : Worker(running)
    , _buffer(buffer)
    , _stations(stations)
    , _current_station(current_station)
    , _current_event(current_event)
    , _printer(printer)
    {}

AudioPacket AudioReceiverWorker::read_packet() {
    char pkt_buf[MTU + 1] = {0};
    ssize_t psize = _data_socket.read(pkt_buf) - 2 * sizeof(uint64_t);
    if (psize < 0)
        throw RadioException("Malformed packet");
    return AudioPacket(pkt_buf, psize);
}

void AudioReceiverWorker::change_station() {
    _data_socket.disable_mcast_recv();
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (!_stations->empty())
        _data_socket.enable_mcast_recv((*_current_station)->get_data_addr());
}

void AudioReceiverWorker::run() {
    bool has_printed = false;
    uint64_t cur_session = NO_SESSION;
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[NETWORK].fd        = _data_socket.fd();
    poll_fds[INTERNAL_EVENT].fd = _current_event->in_fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    for (;;) {
        // this doesn't need a timeout because the remover takes care of inactive stations
        if (-1 == poll(poll_fds, NUM_POLLFDS, -1))
            fatal("poll");

        if (poll_fds[INTERNAL_EVENT].revents & POLLIN) {
            poll_fds[INTERNAL_EVENT].revents = 0;
            auto lock = _current_event.lock();
            auto event = _current_event->get_event();
            if (event == EventPipe::EventType::SIG_INT && !running)
                break;
            if (event == EventPipe::EventType::PACKET_LOSS)
                cur_session = NO_SESSION;
            if (event == EventPipe::EventType::STATION_CHANGE) {
                cur_session = NO_SESSION;
                change_station();
            }
        }
        // got a new packet
        if (poll_fds[NETWORK].revents & POLLIN) {
            try {
                AudioPacket packet = read_packet();
                poll_fds[NETWORK].revents = 0;

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
                    auto lock = _buffer.lock();
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
