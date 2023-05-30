#pragma once

#include "../common/worker.hh"

#include "../common/event_queue.hh"
#include "../common/udp_socket.hh"
#include "../common/circular_buffer.hh"
#include "../common/radio_station.hh"
#include "../common/synced_ptr.hh"

#include "poll.h"
#include <cstddef>

struct AudioReceiverWorker : public Worker {
private:
    UdpSocket _data_socket;
    SyncedPtr<CircularBuffer> _buffer;
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    SyncedPtr<EventQueue> _my_event;
    SyncedPtr<EventQueue> _audio_printer_event;

    AudioPacket read_packet();
    void change_station();
    void handle_audio_packet(const AudioPacket& packet, bool& has_printed, uint64_t& cur_session);
public:
    AudioReceiverWorker() = delete;
    AudioReceiverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& buffer,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventQueue>& my_event,
        const SyncedPtr<EventQueue>& audio_printer_event
    );

    void run() override;
};
