#pragma once

#include "../common/worker.hh"

#include "audio_printer.hh"
#include "../common/event_pipe.hh"
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
    SyncedPtr<EventPipe> _my_event;
    std::shared_ptr<AudioPrinterWorker> _printer;

    AudioPacket read_packet();
    void change_station();
public:
    AudioReceiverWorker() = delete;
    AudioReceiverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& buffer,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const SyncedPtr<EventPipe>& my_event,
        const std::shared_ptr<AudioPrinterWorker>& printer
    );

    void run() override;
};
