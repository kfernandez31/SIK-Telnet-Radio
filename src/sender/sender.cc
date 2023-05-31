#include "sender_params.hh"
#include "retransmitter.hh"
#include "audio_sender.hh"
#include "controller.hh"

#include <thread>

#define RETRANSMITTER 0
#define CONTROLLER    1
#define AUDIO_SENDER  2
#define NUM_WORKERS   3

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
    logger_init(false);
    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    SenderParams params;
    try {
        params = SenderParams(argc, argv);
    } catch (const std::exception& e) {
        fatal(e.what());
    }

    sockaddr_in mcast_addr = get_addr(params.mcast_addr.c_str(), params.data_port);
    auto packet_cache     = SyncedPtr<CircularBuffer>::make(params.fsize);
    packet_cache->reset(params.psize);
    auto rexmit_job_queue = SyncedPtr<std::queue<RexmitRequest>>::make();
    
    std::shared_ptr<Worker> workers[NUM_WORKERS];
    std::thread worker_threads[NUM_WORKERS];

    workers[RETRANSMITTER] = std::make_shared<RetransmitterWorker>(
        running, packet_cache, rexmit_job_queue, 
        event_queues[RETRANSMITTER], params.session_id, params.rtime
    );
    workers[CONTROLLER] = std::make_shared<ControllerWorker>(
        running, event_queues[CONTROLLER], 
        event_queues[RETRANSMITTER], rexmit_job_queue, 
        params.mcast_addr, params.data_port, params.name,
        params.ctrl_port
    );
    workers[AUDIO_SENDER] = std::make_shared<AudioSenderWorker>(
        running, mcast_addr, packet_cache, 
        event_queues[AUDIO_SENDER], params.psize, 
        params.session_id
    );

    for (int i = 0; i < NUM_WORKERS; ++i)
        worker_threads[i] = std::thread([w = workers[i]] { w->run(); });
    
    worker_threads[AUDIO_SENDER].join();
    // when the sender terminiates, the remaining workers should too
    raise(SIGINT);
    for (int i = NUM_WORKERS - 2; i >= 0; --i)
        worker_threads[i].join();

    worker_threads[CONTROLLER].join();
    worker_threads[RETRANSMITTER].join();  

    logger_destroy();
    return 0;
}
