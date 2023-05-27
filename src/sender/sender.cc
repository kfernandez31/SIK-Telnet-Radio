#include "sender_params.hh"
#include "audio_sender.hh"
#include "controller.hh"
#include "../common/circular_buffer.hh"
#include "../common/event_pipe.hh"

#include <thread>

#define RETRANSMITTER 0
#define CONTROLLER    1
#define AUDIO_SENDER  2
#define NUM_WORKERS   3

static volatile sig_atomic_t running = true;
static SyncedPtr<EventPipe> audio_sndr_event = SyncedPtr<EventPipe>::make();

static void sigint_handler(int signum) {
    logerr("Received %s. Shutting down...", strsignal(signum));
    running = false;
    // intentionally not under a mutex
    audio_sndr_event->set_event(EventPipe::EventType::SIG_INT);
}

int main(int argc, char* argv[]) {
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    SenderParams params;
    try {
        params = SenderParams(argc, argv);
    } catch (const std::exception& e) {
        fatal(e.what());
    }

    sockaddr_in data_addr = get_addr(params.mcast_addr.c_str(), params.data_port);
    auto packet_cache     = SyncedPtr<CircularBuffer>::make(params.fsize);
    auto rexmit_job_queue = SyncedPtr<std::queue<RexmitRequest>>::make();
    
    std::shared_ptr<Worker> workers[NUM_WORKERS];
    std::thread worker_threads[NUM_WORKERS];

    workers[RETRANSMITTER] = std::make_shared<RetransmitterWorker>(
        running, packet_cache, rexmit_job_queue,
        params.session_id, params.rtime
    );
    workers[CONTROLLER] = std::make_shared<ControllerWorker>(
        running, data_addr, params.ctrl_port, params.name, 
        std::static_pointer_cast<RetransmitterWorker>(workers[RETRANSMITTER])
    );
    workers[AUDIO_SENDER] = std::make_shared<AudioSenderWorker>(
        running, data_addr, packet_cache, audio_sndr_event,
        params.psize, params.session_id
    );

    for (int i = 0; i < NUM_WORKERS; ++i) 
        worker_threads[i] = std::thread([w = workers[i]] { w->run(); });

    for (int i = NUM_WORKERS - 1; i > 0; --i)
        worker_threads[i].join();

    return 0;
}
