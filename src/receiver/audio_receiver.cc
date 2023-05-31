#include "audio_receiver.hh"

#include "../common/net.hh"
#include "../common/except.hh"
#include "../common/datagram.hh"

#include <poll.h>
#include <unistd.h>

#define NO_SESSION  0

#define MY_EVENT    0
#define NETWORK     1
#define NUM_POLLFDS 2

AudioReceiverWorker::AudioReceiverWorker(
    const volatile sig_atomic_t& running, 
    const SyncedPtr<CircularBuffer>& buffer,
    const SyncedPtr<StationSet>& stations,
    const SyncedPtr<StationSet::iterator>& current_station,
    const SyncedPtr<EventQueue>& my_event,
    const SyncedPtr<EventQueue>& audio_printer_event
)
    : Worker(running, "AudioReceiver")
    , _buffer(buffer)
    , _stations(stations)
    , _current_station(current_station)
    , _my_event(my_event)
    , _audio_printer_event(audio_printer_event)
    {}

AudioPacket AudioReceiverWorker::read_packet() {
    char pkt_buf[UDP_MAX_DATA_SIZE + 1] = {0};
    ssize_t psize = _data_socket.read(pkt_buf, sizeof(pkt_buf) - 1) - 2 * sizeof(uint64_t);
    if (psize < 0)
        throw RadioException("Malformed packet");
    return AudioPacket(pkt_buf, psize);
}

void AudioReceiverWorker::change_station() {
    _data_socket.~UdpSocket();
    auto stations_lock        = _stations.lock();
    auto current_station_lock = _current_station.lock();
    if (!_stations->empty()) {
        _data_socket = UdpSocket();
        _data_socket.enable_mcast_recv((*_current_station)->mcast_addr, (*_current_station)->data_addr);
        _data_socket.bind(ntohs((*_current_station)->data_addr.sin_port));
    }
}

void AudioReceiverWorker::handle_audio_packet(const AudioPacket& packet, bool& has_printed, uint64_t& cur_session) {
    if (packet.session_id < cur_session) {
        log_info("[%s] ignoring old session %llu...", name.c_str(), packet.session_id);
        return;
    }

    if (_buffer->psize() > _buffer->capacity()) {
        log_info("[%s] packet size too large, ignoring session %llu...", name.c_str(), packet.session_id);
        cur_session = NO_SESSION;
        return;
    }

    if (packet.session_id > cur_session) {
        log_info("[%s] new session %llu!", name.c_str(), packet.session_id);
        cur_session = packet.session_id;
        has_printed = false;
        auto lock = _buffer.lock();
        _buffer->reset(packet.psize, packet.first_byte_num);
    }

    if (packet.first_byte_num < _buffer->byte0()) {
        log_info("[%s] packet %zu arrived too late, ignoring...", name.c_str(), packet.first_byte_num);
        return;
    }
    
    auto lock = _buffer.lock();
    _buffer->try_put(std::move(packet));
    if (has_printed || packet.first_byte_num + _buffer->psize() - 1 >= _buffer->printing_threshold()) {
        has_printed |= true;
        _audio_printer_event.lock();
        _audio_printer_event->push(EventQueue::EventType::NEW_JOBS);
    }
}

void AudioReceiverWorker::run() {
    bool has_printed = false;
    uint64_t cur_session = NO_SESSION;
    pollfd poll_fds[NUM_POLLFDS];
    poll_fds[MY_EVENT].fd = _my_event->in_fd();
    poll_fds[NETWORK].fd  = _data_socket.fd();
    for (size_t i = 0; i < NUM_POLLFDS; ++i) {
        poll_fds[i].events  = POLLIN;
        poll_fds[i].revents = 0;
    }

    while (running) {
        if (-1 == poll(poll_fds, NUM_POLLFDS, -1))
            fatal("poll");

        if (poll_fds[MY_EVENT].revents & POLLIN) {
            poll_fds[MY_EVENT].revents = 0;
            EventQueue::EventType event_val = _my_event->pop();
            switch (event_val) {
                case EventQueue::EventType::TERMINATE:
                    return;
                case EventQueue::EventType::CURRENT_STATION_CHANGED:
                    log_info("[%s] changing station", name.c_str());
                    change_station();
                    poll_fds[NETWORK].fd = _data_socket.fd();
                    cur_session = NO_SESSION;
                default: break;
            }
        }

        if (poll_fds[NETWORK].revents & POLLIN) {
            poll_fds[NETWORK].revents = 0;
            try {
                AudioPacket packet = read_packet();
                handle_audio_packet(std::move(packet), has_printed, cur_session);
            } catch (std::exception& e) {
                log_error("[%s] failed to read packet: %s", name.c_str(), e.what());
            }
        }
    }
    log_debug("[%s] going down", name.c_str());
}
