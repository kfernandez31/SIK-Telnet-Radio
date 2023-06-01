#include "receiver_params.hh"

#include "rexmit_sender.hh"
#include "audio_printer.hh"
#include "audio_receiver.hh"
#include "lookup_receiver.hh"
#include "lookup_sender.hh"
#include "station_remover.hh"
#include "ui_menu.hh"

#include <thread>

#define REXMIT_SENDER   0
#define AUDIO_PRINTER   1
#define AUDIO_RECEIVER  2
#define LOOKUP_RECEIVER 3
#define LOOKUP_SENDER   4 // konczy sie
#define STATION_REMOVER 5 // konczy sie
#define UI_MENU         6 // konczy sie
#define NUM_WORKERS     7

static volatile sig_atomic_t running = true;
static bool signalled[NUM_WORKERS];
static SyncedPtr<EventQueue> event_queues[NUM_WORKERS];

static void terminate_worker(const int worker_id) {
    if (!signalled[worker_id]) { // ńecessary check for the handler to be reentrant
        event_queues[worker_id].lock();
        signalled[worker_id] = true;
        event_queues[worker_id]->push(EventQueue::EventType::TERMINATE);
    }
}

static void signal_handler(int signum) {
    log_warn("Received %s. Shutting down...", strsignal(signum));
    for (int i = NUM_WORKERS - 1; i >= 0; --i)
        terminate_worker(i);
    running = false;
}

int main(int argc, char* argv[]) {
    logger_init();

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    ReceiverParams params;
    try {
        params = ReceiverParams(argc, argv);
    } catch (const std::exception& e) {
        fatal(e.what());
    }

    sockaddr_in discover_addr = get_addr(params.discover_addr.c_str(), params.ctrl_port);
    auto stations             = SyncedPtr<StationSet>::make();
    auto current_station      = SyncedPtr<StationSet::iterator>::make(stations->end());
    auto buffer               = SyncedPtr<CircularBuffer>::make(params.bsize);
    auto ctrl_socket          = std::make_shared<UdpSocket>();
    ctrl_socket->set_broadcast();

    std::shared_ptr<Worker> workers[NUM_WORKERS];
    std::thread worker_threads[NUM_WORKERS];

    workers[REXMIT_SENDER] = std::make_shared<RexmitSenderWorker>(
        running, buffer, stations, current_station, params.rtime
    );
    workers[AUDIO_PRINTER] = std::make_shared<AudioPrinterWorker>(
        running, buffer, event_queues[AUDIO_PRINTER]
    );
    workers[AUDIO_RECEIVER] = std::make_shared<AudioReceiverWorker>(
        running, buffer, stations, current_station, 
        event_queues[AUDIO_RECEIVER], event_queues[AUDIO_PRINTER]
    );
    workers[LOOKUP_RECEIVER] = std::make_shared<LookupReceiverWorker>(
        running, stations, current_station, event_queues[LOOKUP_RECEIVER],
        event_queues[AUDIO_RECEIVER], event_queues[UI_MENU], 
        ctrl_socket, params.prio_station_name
    );
    workers[LOOKUP_SENDER] = std::make_shared<LookupSenderWorker>(
        running, event_queues[LOOKUP_SENDER], ctrl_socket, discover_addr
    );
    workers[STATION_REMOVER] = std::make_shared<StationRemoverWorker>(
        running, stations, current_station, event_queues[AUDIO_RECEIVER],
        event_queues[UI_MENU], params.prio_station_name
    );
    workers[UI_MENU] = std::make_shared<UiMenuWorker>(
        running, stations, current_station, event_queues[UI_MENU], 
        event_queues[AUDIO_RECEIVER], params.ui_port
    );

    for (int i = 0; i < NUM_WORKERS; ++i)
        worker_threads[i] = std::thread([w = workers[i]] { w->run(); });

    for (int i = NUM_WORKERS - 1; i >= 0; --i)
        worker_threads[i].join();

    logger_destroy();
    return 0;
}
