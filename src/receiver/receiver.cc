#include "receiver_params.hh"

#include "rexmit_sender.hh"
#include "audio_receiver.hh"
#include "lookup_receiver.hh"
#include "lookup_sender.hh"
#include "station_remover.hh"
#include "tcp_server.hh"

#include <thread>

#define REXMIT_SENDER      0
#define AUDIO_PRINTER      1
#define AUDIO_RECEIVER     2
#define LOOKUP_RECEIVER    3
#define LOOKUP_SENDER      4
#define STATION_REMOVER    5
#define TCP_CLIENT_HANDLER 6
#define TCP_SERVER         7
#define NUM_WORKERS        8

static volatile sig_atomic_t running = true;
static SyncedPtr<EventPipe> audio_recvr_event = SyncedPtr<EventPipe>::make();
static SyncedPtr<EventPipe> cli_handler_event = SyncedPtr<EventPipe>::make();

static void sigint_handler(int signum) {
    logerr("Received %s. Shutting down...", strsignal(signum));
    running = false;
    // intentionally not under a mutex
    audio_recvr_event->set_event(EventPipe::EventType::SIG_INT);
    cli_handler_event->set_event(EventPipe::EventType::SIG_INT);
}

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
    auto client_sockets  = SyncedPtr<TcpClientSocketSet>::make(TcpServerWorker::MAX_CLIENTS);
    auto tcp_poll_fds    = std::make_shared<std::vector<pollfd>>(1 + TcpServerWorker::MAX_CLIENTS);
    for (size_t i = 0; i < tcp_poll_fds->size(); ++i) {
        tcp_poll_fds->at(i).fd      = -1;
        tcp_poll_fds->at(i).events  = POLLIN;
        tcp_poll_fds->at(i).revents = 0;
    }
    tcp_poll_fds->back().fd = cli_handler_event->in_fd();

    workers[REXMIT_SENDER] = std::make_shared<RexmitSenderWorker>(
        running, buffer, stations, current_station, params.rtime
    );
    workers[AUDIO_PRINTER] = std::make_shared<AudioPrinterWorker>(
        running, buffer, audio_recvr_event
    );
    workers[AUDIO_RECEIVER] = std::make_shared<AudioReceiverWorker>(
        running, buffer, stations, current_station, audio_recvr_event,
        std::static_pointer_cast<AudioPrinterWorker>(workers[AUDIO_PRINTER])
    );
    workers[LOOKUP_RECEIVER] = std::make_shared<LookupReceiverWorker>(
        running, stations, current_station, audio_recvr_event,
        cli_handler_event, params.prio_station_name, params.ctrl_port
    );
    workers[LOOKUP_SENDER] = std::make_shared<LookupSenderWorker>(
        running, discover_addr
    );
    workers[STATION_REMOVER] = std::make_shared<StationRemoverWorker>(
        running, stations, current_station, audio_recvr_event,
        cli_handler_event, params.prio_station_name
    );
    workers[TCP_CLIENT_HANDLER] = std::make_shared<TcpClientHandlerWorker>(
        running, stations, current_station, cli_handler_event, 
        audio_recvr_event, client_sockets, tcp_poll_fds
    );
    workers[TCP_SERVER] = std::make_shared<TcpServerWorker>(
        running, params.ui_port, client_sockets, tcp_poll_fds,
        std::static_pointer_cast<TcpClientHandlerWorker>(workers[TCP_CLIENT_HANDLER])
    );
    
    for (int i = 0; i < NUM_WORKERS; ++i)
        worker_threads[i] = std::thread([w = workers[i]] { w->run(); });

    for (int i = NUM_WORKERS - 1; i > 0; --i)
        worker_threads[i].join();

    return 0;
}
