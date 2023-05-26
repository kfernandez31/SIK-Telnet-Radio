#include "receiver_params.hh"

#include "rexmit_sender.hh"
#include "audio_printer.hh"
#include "audio_receiver.hh"
#include "lookup_receiver.hh"
#include "lookup_sender.hh"
#include "station_remover.hh"
#include "ui_menu.hh"

#include "../common/tcp_socket.hh"
#include "../common/circular_buffer.hh"

#include <thread>

#define REXMIT_SENDER   0
#define AUDIO_PRINTER   1
#define AUDIO_RECEIVER  2
#define LOOKUP_RECEIVER 3
#define LOOKUP_SENDER   4
#define STATION_REMOVER 5
#define UI_MENU         6
#define NUM_WORKERS     7

static volatile sig_atomic_t running = true;
static SyncedPtr<EventPipe> current_event = SyncedPtr<EventPipe>::make();

static void sigint_handler(int signum) {
    logerr("Received %s. Shutting down...", strsignal(signum));
    running = false;  
    current_event->put_event(EventPipe::EventType::SIG_INT);
}

//TODO: wielu klientów TCP, wówczas per klient:
// - RexmitSender, UiMenu, AudioReceiver - tak normalnie
// - LookupReceiver, StationRemover      - zbiór klientów i zmienianie każdemu stacji na prio

int main(int argc, char* argv[]) {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    ReceiverParams params;
    try {
        params = ReceiverParams(argc, argv);
    } catch (const std::exception& e) {
        fatal(e.what());
    }

    std::shared_ptr<Worker> workers[NUM_WORKERS];
    std::thread worker_threads[NUM_WORKERS];

    sockaddr_in discover_addr = get_addr(params.discover_addr.c_str(), params.ctrl_port);
    auto stations        = SyncedPtr<StationSet>::make();
    auto current_station = SyncedPtr<StationSet::iterator>::make(stations->end());
    auto buffer          = SyncedPtr<CircularBuffer>::make(params.bsize);
    auto current_event   = SyncedPtr<EventPipe>::make();

    workers[REXMIT_SENDER] = std::make_shared<RexmitSenderWorker>(
        running, buffer, stations, current_station, params.rtime
    );
    workers[AUDIO_PRINTER] = std::make_shared<AudioPrinterWorker>(
        running, buffer, current_event
    );
    workers[AUDIO_RECEIVER] = std::make_shared<AudioReceiverWorker>(
        running, buffer, stations, current_station, current_event,
        std::static_pointer_cast<AudioPrinterWorker>(workers[AUDIO_PRINTER])
    );
    workers[LOOKUP_RECEIVER] = std::make_shared<LookupReceiverWorker>(
        running, stations, current_station, current_event,
        params.prio_station_name, params.ctrl_port
    );
    workers[LOOKUP_SENDER] = std::make_shared<LookupSenderWorker>(
        running, discover_addr
    );
    workers[STATION_REMOVER] = std::make_shared<StationRemoverWorker>(
        running, stations, current_station, current_event,
        params.prio_station_name
    );
    workers[UI_MENU] = std::make_shared<UiMenuWorker>(
        running, stations, current_station, current_event,
        params.ui_port, params.prio_station_name
    );

    for (int i = 0; i < NUM_WORKERS; ++i)
        worker_threads[i] = std::thread([w = workers[i]] { w->run(); });

    for (int i = NUM_WORKERS - 1; i > 0; --i)
        worker_threads[i].join();

    return 0;
}
