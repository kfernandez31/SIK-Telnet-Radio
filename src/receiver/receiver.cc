#include "receiver_params.hh"
#include "rexmit_sender.hh"
#include "audio_printer.hh"
// #include "audio_receiver.hh" //TODO:
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

static int ui_menu_to_audio_receiver_pipe[2];
static int audio_printer_to_audio_receiver_pipe[2];
static int lookup_receiver_to_audio_receiver_pipe[2];
static volatile sig_atomic_t running = true;

static void sigint_handler(int signum) {
    logerr("Received %s. Shutting down...", strsignal(signum));
    running = false;  
}

//TODO: wielu klientów TCP, wówczas per klient:
// - RexmitSender, UiMenu, AudioReceiver - tak normalnie
// - LookupReceiver, StationRemover      - zbiór klientów i zmienianie każdemu stacji na prio

int main(int argc, char* argv[]) {
    if (-1 == pipe(ui_menu_to_audio_receiver_pipe))
        fatal("pipe");
    if (-1 == pipe(audio_printer_to_audio_receiver_pipe))
        fatal("pipe");
    if (-1 == pipe(lookup_receiver_to_audio_receiver_pipe))
        fatal("pipe");
    
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

    auto stations        = SyncedPtr<StationSet>::make();
    auto current_station = SyncedPtr<StationSet::iterator>::make(stations->end());
    auto buffer          = SyncedPtr<CircularBuffer>::make(params.bsize);

    workers[REXMIT_SENDER] = std::make_shared<RexmitSenderWorker>(
        running, params.rtime, buffer, stations, current_station
    );
    workers[AUDIO_PRINTER] = std::make_shared<AudioPrinterWorker>(
        running, buffer, audio_printer_to_audio_receiver_pipe[STDOUT_FILENO]
    );
    // workers[AUDIO_RECEIVER] = std::make_shared<AudioReceiverWorker>(
    //     running, buffer, stations, current_station, 
    //     std::static_pointer_cast<AudioPrinterWorker>(workers[AUDIO_PRINTER]), 
    //     audio_printer_to_audio_receiver_pipe[STDIN_FILENO], 
    //     ui_menu_to_audio_receiver_pipe[STDIN_FILENO], 
    //     lookup_receiver_to_audio_receiver_pipe[STDIN_FILENO]
    // );
    workers[LOOKUP_RECEIVER] = std::make_shared<LookupReceiverWorker>(
        running, stations, current_station, params.prio_station_name, 
        lookup_receiver_to_audio_receiver_pipe[STDOUT_FILENO]
    );
    workers[LOOKUP_SENDER] = std::make_shared<LookupSenderWorker>(
        running, params.ctrl_port, params.discover_addr
    );
    workers[STATION_REMOVER] = std::make_shared<StationRemoverWorker>(
        running, stations, current_station, params.prio_station_name
    );
    workers[UI_MENU] = std::make_shared<UiMenuWorker>(
        running, params.ui_port, params.prio_station_name, stations, 
        current_station, ui_menu_to_audio_receiver_pipe[STDOUT_FILENO]
    );

    for (int i = 0; i < NUM_WORKERS; ++i)
        worker_threads[i] = std::thread([w = workers[i]] { w->run(); });

    for (int i = NUM_WORKERS - 1; i > 0; --i)
        worker_threads[i].join();

    return 0;
}
