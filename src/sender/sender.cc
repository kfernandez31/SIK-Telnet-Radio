#include "sender_params.hh"
#include "audio_sender.hh"
#include "../common/circular_buffer.hh"

#include <thread>

#define AUDIO_SENDER  0
#define CONTROLLER    1
#define RETRANSMITTER 2
#define NUM_WORKERS   1

int sender_pipe[2];
volatile sig_atomic_t running = true;

static void sigint_handler(int signum) {
    logerr("Received %s. Shutting down...", strsignal(signum));
    running = false;
    if (-1 == close(sender_pipe[1]))
        fatal("close");
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

    sockaddr_in mcast_addr = get_addr(params.mcast_addr.c_str(), params.data_port);
    auto packet_cache = SyncedPtr<CircularBuffer>::make(params.fsize);
    
    std::shared_ptr<Worker> workers[NUM_WORKERS];
    std::thread worker_threads[NUM_WORKERS];

    workers[AUDIO_SENDER] = std::make_shared<AudioSenderWorker>(running, std::ref(mcast_addr), params.psize, params.session_id, packet_cache);
    worker_threads[AUDIO_SENDER] = std::thread([worker = workers[AUDIO_SENDER]] {
        worker->run();
    });

    pipe(sender_pipe);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    worker_threads[AUDIO_SENDER].join();
    raise(SIGINT); // to make others stop

    // worker_threads[RETRANSMITTER].join();
    // worker_threads[CONTROLLER].join();

    return 0;
}
