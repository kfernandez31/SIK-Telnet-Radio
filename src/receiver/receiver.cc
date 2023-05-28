#include "receiver_params.hh"

#include "rexmit_sender.hh"
#include "audio_printer.hh"
#include "audio_receiver.hh"
#include "lookup_receiver.hh"
#include "lookup_sender.hh"
#include "station_remover.hh"
#include "tcp_server.hh"

#include <thread>

int EXCEPTION_THREAD = 6;
int HANGING_THREAD = 7;

#define REXMIT_SENDER   0
#define AUDIO_PRINTER   1
#define AUDIO_RECEIVER  2
#define LOOKUP_RECEIVER 3
#define LOOKUP_SENDER   4
#define STATION_REMOVER 5
#define UI_MENU         6
#define TCP_SERVER      7
#define NUM_WORKERS     8

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

static void signal_handler(int signum) { //TODO: to samo w senderze
    logerr("Received %s. Shutting down...", strsignal(signum));
    for (int i = NUM_WORKERS - 1; i >= 0; --i)
        if (i != EXCEPTION_THREAD && i != HANGING_THREAD)
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

    std::shared_ptr<Worker> workers[NUM_WORKERS];
    std::thread worker_threads[NUM_WORKERS];

    sockaddr_in discover_addr = get_addr(params.discover_addr.c_str(), params.ctrl_port);
    auto stations        = SyncedPtr<StationSet>::make();
    auto current_station = SyncedPtr<StationSet::iterator>::make(stations->end());
    auto buffer          = SyncedPtr<CircularBuffer>::make(params.bsize);
    auto client_sockets  = SyncedPtr<TcpClientSocketSet>::make(TcpServerWorker::MAX_CLIENTS);
    auto tcp_poll_fds    = std::make_shared<std::vector<pollfd>>(1 + TcpServerWorker::MAX_CLIENTS);
    for (size_t i = 0; i < tcp_poll_fds->size(); ++i) {
        tcp_poll_fds->at(i).fd      = -1;
        tcp_poll_fds->at(i).events  = POLLIN;
        tcp_poll_fds->at(i).revents = 0;
    }
    tcp_poll_fds->back().fd = event_queues[UI_MENU]->in_fd();

    workers[REXMIT_SENDER] = std::make_shared<RexmitSenderWorker>(
        running, buffer, stations, current_station, params.rtime
    );
    workers[AUDIO_PRINTER] = std::make_shared<AudioPrinterWorker>(
        running, buffer, event_queues[AUDIO_PRINTER], 
        event_queues[AUDIO_RECEIVER]
    );
    workers[AUDIO_RECEIVER] = std::make_shared<AudioReceiverWorker>(
        running, buffer, stations, current_station, 
        event_queues[AUDIO_RECEIVER], event_queues[AUDIO_PRINTER]
    );
    workers[LOOKUP_RECEIVER] = std::make_shared<LookupReceiverWorker>(
        running, stations, current_station, event_queues[LOOKUP_RECEIVER],
        event_queues[AUDIO_RECEIVER], event_queues[UI_MENU], 
        params.prio_station_name, params.ctrl_port
    );
    workers[LOOKUP_SENDER] = std::make_shared<LookupSenderWorker>(
        running, discover_addr
    );
    workers[STATION_REMOVER] = std::make_shared<StationRemoverWorker>(
        running, stations, current_station, event_queues[AUDIO_RECEIVER],
        event_queues[UI_MENU], params.prio_station_name
    );
    workers[UI_MENU] = std::make_shared<UiMenuWorker>(
        running, stations, current_station, event_queues[UI_MENU], 
        event_queues[AUDIO_RECEIVER], client_sockets, tcp_poll_fds
    );
    workers[TCP_SERVER] = std::make_shared<TcpServerWorker>(
        running, client_sockets, event_queues[TCP_SERVER], tcp_poll_fds,
        std::static_pointer_cast<UiMenuWorker>(workers[UI_MENU]),    
        params.ui_port
    );

    for (int i = 0; i < NUM_WORKERS; ++i) {
        log_debug("i = %d", i);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        if (i != 4 && i != EXCEPTION_THREAD && i != HANGING_THREAD)
            worker_threads[i] = std::thread([w = workers[i]] { w->run(); });
        log_debug("i = %d", i);
    }

    for (int i = NUM_WORKERS - 1; i >= 0; --i)
        if (i != EXCEPTION_THREAD && i != HANGING_THREAD)
            worker_threads[i].join();

    logger_destroy();
    return 0;
}
