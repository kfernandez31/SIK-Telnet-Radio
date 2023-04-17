#include "utils/err.hh"
#include "utils/buffer.hh"
#include "utils/endian.hh"
#include "utils/audio_packet.hh"
#include "utils/net.hh"
#include "utils/alloc.hh"

#include <unistd.h>
#include <csignal>

#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <set>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#define NO_SESSION 0
#define MAX_PKT_SIZE (1 << 16)

using interval = std::pair<uint64_t, uint64_t>;

struct receiver_params {
    std::string src_addr;
    uint16_t    data_port;
    size_t      bsize;
};

enum flusher_state_t {
    WORK,
    DONE,
    WAIT,
};

int exit_code;
bool running = true;

static receiver_params get_params(int argc, char* argv[]) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",  "produce help message")
        ("addr,a",  bpo::value<std::string>(), "SRC_ADDR")
        ("port,P",  bpo::value<uint16_t>()->default_value(29629), "DATA_PORT")
        ("bsize,b", bpo::value<size_t>()->default_value(65536), "BSIZE");

    bpo::variables_map vm;
    try {
        bpo::store(
            bpo::command_line_parser(argc, argv)
                .options(desc)
                .style(bpo::command_line_style::unix_style | bpo::command_line_style::allow_long_disguise)
                .run()
            , vm 
        );
        bpo::notify(vm);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n' << desc;
        exit(EXIT_FAILURE);
    }

    if (vm.count("help")) {
        std::cout << desc;
        exit(EXIT_SUCCESS);
    }

    if (!vm.count("addr")) {
        fatal("DEST_ADDR is required.");
    }

    return receiver_params {
        .src_addr  = vm["addr"].as<std::string>(),
        .data_port = vm["port"].as<uint16_t>(),
        .bsize     = vm["bsize"].as<size_t>(),
    };
}

static int bind_socket(const uint16_t port) {
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    ENSURE(socket_fd > 0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(port);

    // bind the socket to a concrete address
    CHECK_ERRNO(bind(socket_fd, (struct sockaddr*)&server_address, (socklen_t)sizeof(server_address)));
    return socket_fd;
}

static size_t read_packet(const int socket_fd, const size_t bsize, bool& connected, struct sockaddr_in* sender_addr, audio_packet* pkt) {
    ssize_t nread;
    if (!connected) {
        socklen_t addr_len = sizeof(*sender_addr);
        nread = recvfrom(socket_fd, pkt, bsize, 0, (struct sockaddr*)sender_addr, &addr_len);
        connect(socket_fd, (struct sockaddr*)sender_addr, addr_len);
        connected = true;
    } else
        nread = recv(socket_fd, pkt, bsize, 0);
    VERIFY(nread);
    ENSURE((size_t)nread > 2 * sizeof(uint64_t));
    pkt->first_byte_num = ntohll(pkt->first_byte_num);
    pkt->session_id     = ntohll(pkt->session_id);
    return nread - 2 * sizeof(uint64_t);
}

static uint64_t flush_threshold(const uint64_t byte0, const size_t bsize) {
    return byte0 + bsize * 3 / 4;
}

static void signal_handler(int signum) {
    eprintln("Received %s. Shutting down...", strsignal(signum));
    running = false;
    exit_code = signum;
}

// the flusher's variables are intentionally not under shared_pointers, as they're guaranteed to be valid
static void flushing_work(
    const uint8_t* buffer,
    const size_t bsize,
    const size_t& rel_tail,
    size_t& start,
    size_t& nbytes,
    flusher_state_t& state,
    std::condition_variable& flusher_waiting_for_main_cv, 
    std::condition_variable& main_waiting_for_flusher_cv,
    std::mutex& flusher_mtx
) {
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(flusher_mtx);
            flusher_waiting_for_main_cv.wait(lock, [&] { return state != flusher_state_t::WAIT; });
        }
        if (state == flusher_state_t::DONE)
            return;
        
        cyclical_write(STDOUT_FILENO, buffer + rel_tail, bsize, start, nbytes);
        {
            std::unique_lock<std::mutex> lock(flusher_mtx);
            state = flusher_state_t::WAIT;
        }
        main_waiting_for_flusher_cv.notify_one();
    }
}

static void wait_if_being_flushed(
    const bool wait_unconditionally,
    const size_t flush_start,
    const size_t flush_end,
    const size_t bsize,
    const size_t offset,
    const flusher_state_t& flusher_state,
    std::condition_variable& main_waiting_for_flusher_cv,
    std::mutex& flusher_mtx
) {
    // wait if the current slot is being flushed
    std::unique_lock<std::mutex> lock(flusher_mtx);
    if (flusher_state == flusher_state_t::WORK) {
        if (wait_unconditionally || is_between(flush_start, flush_end, bsize, offset)) {
            main_waiting_for_flusher_cv.wait(lock, [&] { return flusher_state == flusher_state_t::WAIT; });
        }
    }
}

static void split_pending(std::set<interval>& pending, const size_t psize, const size_t packet_id) {
    auto it = pending.lower_bound({packet_id, packet_id});

    if (it != pending.begin() && packet_id <= std::prev(it)->second) 
        it = std::prev(it);

    // if the value is in an existing interval, split it into two parts
    if (it != pending.end() && it->first <= packet_id && packet_id <= it->second) {
        uint64_t left  = it->first;
        uint64_t right = it->second;
        pending.erase(it);
        if (left < packet_id) 
            pending.insert({left, packet_id - psize});
        if (packet_id < right) 
            pending.insert({packet_id + psize, right});
    }
}

static void remove_pending(std::set<interval>& pending, const size_t psize, const uint64_t bound) {
    auto it = pending.lower_bound({bound, bound});
    if (it != pending.begin()) { // get the first interval in which the bound might be
        auto prev = std::prev(it);
        if (prev->first <= bound && bound <= prev->second)
            it = prev;
        else
            pending.erase(pending.begin(), it);
    }
    uint64_t left  = it->first;
    uint64_t right = it->second;
    if (left <= bound && bound <= right) {
        pending.erase(pending.begin(), ++it);
        if (bound + psize <= right)
            pending.insert({bound + psize, right});
    }
}

static void print_pending(const std::set<interval>& pending, const size_t packet_id) {
    for (const auto& [l, r] : pending)
        for (size_t i = l; i <= r; i++)
            eprintln("MISSING: BEFORE %zu EXPECTED %zu", packet_id, i);
}

static std::optional<size_t> get_write_pos(const audio_packet& pkt, std::set<interval>& pending, const size_t bsize, const size_t psize, uint64_t& abs_head, uint64_t& abs_tail, size_t& rel_head, size_t& rel_tail) {
    size_t head_offset = pkt.first_byte_num - abs_head;
    size_t tail_offset = pkt.first_byte_num - abs_tail;
    if (pkt.first_byte_num < abs_tail) // dismiss, packet is too far behind
        return {};
    if (pkt.first_byte_num > abs_head) {
        if (head_offset >= bsize) // dismiss, packet is too far ahead //TODO: needed?
            return {};
        size_t   new_rel_head = (rel_head + head_offset) % bsize + rel_tail;
        uint64_t new_abs_head = pkt.first_byte_num + psize;
        if (is_between(rel_tail, rel_head, bsize, new_rel_head)) { // advance, overwriting old data
            print_pending(pending, pkt.first_byte_num);
            size_t new_abs_tail = abs_tail + head_offset;
            size_t new_rel_tail = (rel_tail + head_offset) % bsize + rel_tail;
            remove_pending(pending, psize, new_abs_tail - psize);
            abs_tail = new_abs_tail;
            rel_tail = new_rel_tail;
        } else { // advance normally
            if (new_abs_head - abs_head >= 2 * psize) // there is a gap
                pending.insert({abs_head, new_abs_head - 2 * psize});
            print_pending(pending, pkt.first_byte_num);
        }
        abs_head  = new_abs_head;
        rel_head  = new_rel_head;
        return new_rel_head;
    } else { // fill in a gap
        split_pending(pending, psize, pkt.first_byte_num);
        return (rel_tail + tail_offset) % bsize + rel_tail;
    }
}

static int run(const receiver_params* params) {
    uint64_t abs_head, abs_tail /*this is "byte0"*/, cur_session = NO_SESSION;
    size_t   rel_head, rel_tail, flush_start, flush_end, to_flush, psize;
    bool     connected = false;
    struct sockaddr_in sender_addr = get_addr(params->src_addr.c_str(), {});
    
    int socket_fd = bind_socket(params->data_port);
    eprintln("Listening on port %u...", params->data_port);

    uint8_t* buffer = (uint8_t*)checked_calloc(params->bsize, sizeof(uint8_t));
    
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    VERIFY(sigaction(SIGINT, &sa, NULL));

    std::set<interval> pending;
    std::mutex flusher_mtx;
    std::condition_variable flusher_waiting_for_main_cv, main_waiting_for_flusher_cv;
    auto flusher_state = flusher_state_t::WAIT;
    std::thread flusher([&] {
        flushing_work(buffer, params->bsize, rel_tail, flush_start, to_flush, 
            flusher_state, flusher_waiting_for_main_cv, main_waiting_for_flusher_cv, flusher_mtx);
    });

    while (running) { //TODO: handling przerwań
        audio_packet pkt;
        psize = read_packet(socket_fd, params->bsize, connected, &sender_addr, &pkt);

        if (pkt.session_id < cur_session) {
            eprintln("Ignoring old session %llu...", pkt.session_id);
            continue;
        }

        if (pkt.session_id > cur_session) {
            cur_session = pkt.session_id;
            abs_head    = abs_tail = pkt.first_byte_num;
            rel_head    = rel_tail = 0;
            pending.clear();
            wait_if_being_flushed(true, 0, 0, 0, 0, flusher_state, main_waiting_for_flusher_cv, flusher_mtx);
            memset(buffer, 0, params->bsize * sizeof(uint8_t));
            eprintln("New session %llu...", pkt.session_id);
        }
        
        eprintln("Received packet %zu", pkt.first_byte_num);

        auto write_pos = get_write_pos(pkt, pending, params->bsize, psize, abs_head, abs_tail, rel_head, rel_tail);
        if (!write_pos)
            continue;
        
        wait_if_being_flushed(false, flush_start, flush_end, params->bsize, write_pos.value(), flusher_state, main_waiting_for_flusher_cv, flusher_mtx);
        cyclical_memcpy(buffer + rel_tail, pkt.audio_data, params->bsize, write_pos.value(), psize);

        if (abs_head - 1 >= flush_threshold(abs_tail, params->bsize)) { // try to flush
            wait_if_being_flushed(true, 0, 0, 0, 0, flusher_state, main_waiting_for_flusher_cv, flusher_mtx);
            to_flush      = abs_head - abs_tail;
            flush_start   = rel_tail;
            flush_end     = (rel_tail + to_flush) % params->bsize + rel_tail;
            flusher_state = flusher_state_t::WORK;
            flusher_waiting_for_main_cv.notify_one();
        }
    }

    wait_if_being_flushed(true, 0, 0, 0, 0, flusher_state, main_waiting_for_flusher_cv, flusher_mtx);
    flusher_state = flusher_state_t::DONE;
    flusher_waiting_for_main_cv.notify_one();
    flusher.join();
    free(buffer);
    CHECK_ERRNO(close(socket_fd));
    return exit_code;
}

int main(int argc, char* argv[]) {
    receiver_params params = get_params(argc, argv);
    return run(&params);
}
