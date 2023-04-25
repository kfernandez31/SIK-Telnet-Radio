#include "utils/err.hh"
#include "utils/endian.hh"
#include "utils/audio_packet.hh"
#include "utils/net.hh"
#include "utils/cyclical_buffer.hh"

#include <unistd.h>
#include <csignal>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#define NO_SESSION 0
#define MAX_UDP_PKT_SIZE ((1 << 16) - 1)
#define MAX_UINT64_T_DIGITS 20
#define LOG_MSG_LEN (sizeof("MISSING: BEFORE  EXPECTED  \n") - 1)
#define TOTAL_LOG_LEN (2 * MAX_UINT64_T_DIGITS + LOG_MSG_LEN)

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

int socket_fd;
flusher_state_t flusher_state;
std::condition_variable flusher_cv;
std::thread flusher;

static receiver_params get_params(int argc, char* argv[]) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",  "produce help message")
        ("addr,a",  bpo::value<std::string>(), "SRC_ADDR")
        ("port,P",  bpo::value<uint16_t>()->default_value(29629), "DATA_PORT")
        ("bsize,b", bpo::value<size_t>()->default_value(65536), "BSIZE");

    bpo::variables_map vm;
    try {
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n' << desc;
        exit(EXIT_FAILURE);
    }

    if (vm.count("help")) {
        std::cout << desc;
        exit(EXIT_SUCCESS);
    }

    if (!vm.count("addr"))
        fatal("SRC_ADDR is required.");

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
    server_address.sin_family      = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port        = htons(port);

    // bind the socket to a concrete address
    CHECK_ERRNO(bind(socket_fd, (struct sockaddr*)&server_address, (socklen_t)sizeof(server_address)));
    return socket_fd;
}

static size_t read_packet(const int socket_fd, struct sockaddr_in* sender_addr, uint8_t* pkt_buf) {
    static bool connected = false;
    ssize_t nread;
    if (!connected) {
        socklen_t addr_len = sizeof(*sender_addr);
        nread = recvfrom(socket_fd, pkt_buf, MAX_UDP_PKT_SIZE, 0, (struct sockaddr*)sender_addr, &addr_len);
        connect(socket_fd, (struct sockaddr*)sender_addr, addr_len);
        connected = true;
    } else
        nread = recv(socket_fd, pkt_buf, MAX_UDP_PKT_SIZE, 0);
    VERIFY(nread);
    ENSURE((size_t)nread > 2 * sizeof(uint64_t));
    return nread - 2 * sizeof(uint64_t);
}

static void cleanup() {
    flusher_state = flusher_state_t::DONE;
    flusher_cv.notify_one();
    flusher.join();
    CHECK_ERRNO(close(socket_fd));
}

static void signal_handler(int signum) {
    eprintln("Received %s. Shutting down...", strsignal(signum));
    cleanup();
    exit(signum); //TODO: this leaks
}

// the flusher's variables are intentionally not under shared_pointers, as they're guaranteed to be valid
static void flushing_work(
    cyclical_buffer& buf,
    size_t& nbytes,
    flusher_state_t& state,
    std::mutex& buf_mtx
) {
    for (;;) {
        std::unique_lock<std::mutex> lock(buf_mtx);
        //eprintln("flusher --- going to sleep");
        flusher_cv.wait(lock, [&] { return state != flusher_state_t::WAIT; });
        if (state == flusher_state_t::DONE)
            break;
        //eprintln("flusher --- starting work");
        buf.dump(nbytes);
        state = flusher_state_t::WAIT;
    }
    //eprintln("flusher --- dying!");
}

static void print_pending(const cyclical_buffer& buf, const uint64_t abs_head, const size_t cur_pkt) {
    uint64_t abs_tail  = abs_head - buf.range();
    size_t head_offset = cur_pkt - abs_head;
    size_t tail_offset = cur_pkt - abs_tail;
    size_t missing_cnt = head_offset / buf.psize;
    size_t last_pkt    = (buf.tail + tail_offset + buf.rounded_cap() - buf.psize) % buf.rounded_cap();
    
    //eprintln("[15] abs_tail = %zu", abs_tail);
    //eprintln("[15] head_offset = %zu", head_offset);
    //eprintln("[16] tail_offset = %zu", tail_offset);
    //eprintln("[17] missing_cnt = %zu", missing_cnt);
    //eprintln("[18] last_pkt = %zu", last_pkt);

    if (abs_tail < cur_pkt)
        for (size_t i = buf.tail; i != last_pkt; i = (i + buf.psize) % buf.rounded_cap()) {
            //eprintln("[19] i = %zu", i);
            missing_cnt += !buf.populated[i];
        }

    char log_buf[missing_cnt * TOTAL_LOG_LEN];
    const char* fmt = "MISSING: BEFORE %llu EXPECTED %llu \n";

    size_t len = 0;
    if (abs_tail < cur_pkt)
        for (size_t i = buf.tail; i != last_pkt; i = (i + buf.psize) % buf.rounded_cap())
            if (!buf.populated[i]) {
                size_t nprinted = snprintf(log_buf + len, TOTAL_LOG_LEN, fmt, cur_pkt, abs_tail + i);
                VERIFY(nprinted);
                len += nprinted;
            }
    for (size_t i = 0; i < head_offset; i += buf.psize) {
        size_t nprinted = snprintf(log_buf + len, TOTAL_LOG_LEN, fmt, cur_pkt, abs_head + i);
        VERIFY(nprinted);
        len += nprinted;
    }

    ssize_t nwritten = write(STDERR_FILENO, log_buf, len);
    VERIFY(nwritten);
    ENSURE((ssize_t)len == nwritten);
}

// head_byte_num == abs_head
static void try_write_packet(const uint64_t first_byte_num, const uint8_t* audio_data, cyclical_buffer& buf, uint64_t& abs_head) {
    uint64_t abs_tail  = abs_head - buf.range();
    if (first_byte_num < abs_tail) // dismiss, packet is too far behind
    {
        //eprintln("  try_write_packet --- dismiss");
        return;
    }
    if (first_byte_num >= abs_head) { // advance
        //eprintln("  try_write_packet --- advance");
        size_t head_offset = first_byte_num - abs_head;
        print_pending(buf, abs_head, first_byte_num);
        buf.advance(audio_data, head_offset);
        abs_head = first_byte_num + buf.psize;
    } else { // fill in a gap
        //eprintln("  try_write_packet --- fill_gap");
        size_t tail_offset = first_byte_num - abs_tail;
        buf.fill_gap(audio_data, tail_offset);
    }
}

static void run(const receiver_params* params) {
    uint64_t abs_head, byte0, cur_session = NO_SESSION;
    size_t   to_flush;
    bool     first_flush;
    struct sockaddr_in sender_addr = get_addr(params->src_addr.c_str(), NULL);
    socket_fd = bind_socket(params->data_port);
    eprintln("Listening on port %u...", params->data_port);
    cyclical_buffer buf(params->bsize);
    signal(SIGINT, signal_handler);

    std::mutex buf_mtx;
    flusher_state = flusher_state_t::WAIT;
    flusher = std::thread([&] { flushing_work(buf, to_flush, flusher_state, buf_mtx); });

    uint8_t pkt_buf[MAX_UDP_PKT_SIZE];
    for (size_t it = 0;; it++) {
        //eprintln("");
        //eprintln("=== ITERATION %zu ===", it);
        size_t   new_psize      = read_packet(socket_fd, &sender_addr, pkt_buf);
        uint64_t session_id     = get_session_id(pkt_buf);
        uint64_t first_byte_num = get_first_byte_num(pkt_buf);
        uint8_t* audio_data     = get_audio_data(pkt_buf);

        if (session_id < cur_session) {
            //eprintln("[2]  Ignoring old session %llu...", session_id);
            continue;
        }

        if (buf.psize > params->bsize)
            continue;

        if (session_id > cur_session) {
            //eprintln("[3]  New session %llu...", session_id);
            cur_session = session_id;
            abs_head    = byte0 = first_byte_num;
            first_flush = true;
            std::unique_lock<std::mutex> lock(buf_mtx);
            buf.reset(new_psize);
            //eprintln("[4]  Resetted variables");
        }

        if (first_byte_num < byte0) {
            //eprintln("[2]  Ignoring packet...", session_id);
            continue;
        }
        
        std::unique_lock<std::mutex> lock(buf_mtx);
        //eprintln("run --- try_write_packet(%llu, _, _, %llu)", first_byte_num, abs_head);
        //eprintln("run --- buffer state (before):    tail = %zu,    head = %zu,     range = %zu", buf.tail, buf.head, buf.range());
        try_write_packet(first_byte_num, audio_data, buf, abs_head);
        //eprintln("run --- buffer state (after):    tail = %zu,    head = %zu,     range = %zu", buf.tail, buf.head, buf.range());

        if (!first_flush || (first_flush && (first_byte_num + buf.psize - 1) >= (byte0 + params->bsize / 4 * 3))) {
            // size_t cnt = 0;
            // for (size_t i = 0; i < buf.rounded_cap(); i += buf.psize)
            //     cnt += buf.populated[i];
            // eprintln("Buffer's range = %zu, population: %zu", buf.range(), cnt);

            first_flush   = false;
            to_flush      = buf.range();
            flusher_state = flusher_state_t::WORK;
            //eprintln("[6]  Delegating flush:  to_flush = %llu", buf.range());
            lock.unlock();
            flusher_cv.notify_one();
        } else {
            //eprintln("[7] Did not flush");
        }
    }

    std::unique_lock<std::mutex> lock(buf_mtx);
    cleanup();
}

int main(int argc, char* argv[]) {
    receiver_params params = get_params(argc, argv);
    run(&params);
    return 0;
}
