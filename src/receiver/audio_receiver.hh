#pragma once

#include "../common/worker.hh"

#include "audio_printer.hh"
#include "../common/udp_socket.hh"
#include "../common/circular_buffer.hh"
#include "../common/radio_station.hh"
#include "../common/ptr.hh"

#include <cstddef>

#define NUM_POLLFDS 4

//TODO: jak dojdzie PIERWSZA stacja, to na nią przełączyć!

struct AudioReceiverWorker : public Worker {
private:
    UdpSocket _data_socket;
    SyncedPtr<CircularBuffer> _buffer;
    SyncedPtr<StationSet> _stations;
    SyncedPtr<StationSet::iterator> _current_station;
    std::shared_ptr<AudioPrinterWorker> _printer;
    pollfd _poll_fds[NUM_POLLFDS];

    AudioPacket read_packet();
    void AudioReceiverWorker::try_write_packet(const AudioPacket& packet, uint64_t& abs_head);
public:
    AudioReceiverWorker() = delete;
    ~AudioReceiverWorker();
    AudioReceiverWorker(
        const volatile sig_atomic_t& running, 
        const SyncedPtr<CircularBuffer>& buffer,
        const SyncedPtr<StationSet>& stations,
        const SyncedPtr<StationSet::iterator>& current_station,
        const std::shared_ptr<AudioPrinterWorker>& printer,
        //TODO: rexmitsender
        const int printer_fd,
        const int menu_fd,
        const int lookup_fd
    );

    void run() override;
};
