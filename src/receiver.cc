#include "utils/err.hh"
#include "utils/endian.hh"
#include "utils/audio_packet.hh"
#include "utils/net.hh"
#include "utils/mem.hh"
#include "utils/cyclical_buffer.hh"

#include <unistd.h>
#include <csignal>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

#define NO_SESSION 0
#define MAX_UDP_PKT_SIZE ((1 << 16) - 1)
#define MAX_UINT64_T_DIGITS 20
#define LOG_MSG_LEN (sizeof("MISSING: BEFORE  EXPECTED  \n") - 1)
#define TOTAL_LOG_LEN (2 * MAX_UINT64_T_DIGITS + LOG_MSG_LEN)

struct receiver_params {
    std::string src_addr;
    uint16_t    data_port;
    size_t      bsize;
};

enum printer_state_t {
    WORK,
    DONE,
    WAIT,
};

int socket_fd;
std::atomic<printer_state_t> printer_state;
std::condition_variable printer_cv;
std::thread printer;
cyclical_buffer* buf_ptr;

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
    printer_state.store(printer_state_t::DONE);
    printer_cv.notify_one();
    printer.join();
    if (buf_ptr)
        buf_ptr->~cyclical_buffer();
    CHECK_ERRNO(close(socket_fd));
}

static void signal_handler(int signum) {
    eprintln("Received %s. Shutting down...", strsignal(signum));
    cleanup();
    exit(signum);
}

// the references the printer holds are guaranteed to be valid
static void printing_work(
    cyclical_buffer& buf,
    std::mutex& buf_mtx
) {
    for (;;) {
        printer_state.store(printer_state_t::WAIT);
        std::unique_lock<std::mutex> lock(buf_mtx);
        printer_cv.wait(lock, [&] { return printer_state.load() != printer_state_t::WAIT; });
        if (printer_state.load() == printer_state_t::DONE)
            break;        
        buf.dump_tail(buf.psize() * buf.cnt_upto_gap());
    }
}

static void print_missing(const cyclical_buffer& buf, const uint64_t abs_head, const size_t cur_pkt) {
    uint64_t abs_tail  = abs_head - buf.range();
    size_t head_offset = cur_pkt - abs_head;
    size_t tail_offset = cur_pkt - abs_tail;
    size_t missing_cnt = head_offset / buf.psize();
    size_t last_pkt    = (buf.tail() + tail_offset + buf.rounded_cap() - buf.psize()) % buf.rounded_cap();
    
    //eprintln("[15] abs_tail = %zu", abs_tail);
    //eprintln("[15] head_offset = %zu", head_offset);
    //eprintln("[16] tail_offset = %zu", tail_offset);
    //eprintln("[17] missing_cnt = %zu", missing_cnt);
    //eprintln("[18] last_pkt = %zu", last_pkt);

    if (abs_tail < cur_pkt)
        for (size_t i = buf.tail(); i != last_pkt; i = (i + buf.psize()) % buf.rounded_cap())
            missing_cnt += !buf.occupied(i);

    char log_buf[missing_cnt * TOTAL_LOG_LEN];
    const char* fmt = "MISSING: BEFORE %llu EXPECTED %llu \n";

    size_t len = 0;
    if (abs_tail < cur_pkt)
        for (size_t i = buf.tail(); i != last_pkt; i = (i + buf.psize()) % buf.rounded_cap())
            if (!buf.occupied(i)) {
                ssize_t nprinted = snprintf(log_buf + len, TOTAL_LOG_LEN, fmt, cur_pkt, abs_tail + i);
                VERIFY(nprinted);
                len += (size_t)nprinted;
            }
    for (size_t i = 0; i < head_offset; i += buf.psize()) {
        ssize_t nprinted = snprintf(log_buf + len, TOTAL_LOG_LEN, fmt, cur_pkt, abs_head + i);
        VERIFY(nprinted);
        len += (size_t)nprinted;
    }

    ssize_t nwritten = write(STDERR_FILENO, log_buf, len);
    VERIFY(nwritten);
    ENSURE((ssize_t)len == nwritten);
}

static void try_write_packet(const uint64_t first_byte_num, const uint8_t* audio_data, cyclical_buffer& buf, uint64_t& abs_head) {
    uint64_t abs_tail  = abs_head - buf.range();
    if (first_byte_num < abs_tail) // dismiss, packet is too far behind
        return;
    if (first_byte_num >= abs_head) { // advance
        size_t head_offset = first_byte_num - abs_head;
        print_missing(buf, abs_head, first_byte_num);
        buf.push_head(audio_data, head_offset);
        abs_head = first_byte_num + buf.psize();
    } else { // fill in a gap
        size_t tail_offset = first_byte_num - abs_tail;
        buf.fill_gap(audio_data, tail_offset);
    }
}

static void run(const receiver_params* params) {
    uint64_t abs_head, byte0, cur_session = NO_SESSION;
    bool     first_print;
    struct sockaddr_in sender_addr = get_addr(params->src_addr.c_str(), NULL);
    socket_fd = bind_socket(params->data_port);
    eprintln("\nListening on port %u...", params->data_port);
    cyclical_buffer buf(params->bsize);
    buf_ptr = &buf;
    signal(SIGINT, signal_handler);

    std::mutex buf_mtx;
    printer_state = printer_state_t::WAIT;
    printer = std::thread([&] { printing_work(buf, buf_mtx); });

    uint8_t pkt_buf[MAX_UDP_PKT_SIZE];
    for (;;) {
        size_t   new_psize      = read_packet(socket_fd, &sender_addr, pkt_buf);
        uint64_t session_id     = get_session_id(pkt_buf);
        uint64_t first_byte_num = get_first_byte_num(pkt_buf);
        uint8_t* audio_data     = get_audio_data(pkt_buf);

        if (session_id < cur_session) {
            eprintln("Ignoring old session %llu...", session_id);
            continue;
        }

        if (buf.psize() > params->bsize) {
            eprintln("Packet size too large, ignoring session  %llu...", session_id);
            continue;
        }

        if (session_id > cur_session) {
            eprintln("New session %llu!", session_id);
            cur_session = session_id;
            abs_head    = byte0 = first_byte_num;
            first_print = true;
            std::unique_lock<std::mutex> lock(buf_mtx);
            buf.reset(new_psize);
        }

        if (first_byte_num < byte0)
            continue;
        
        {
            std::unique_lock<std::mutex> lock(buf_mtx);
            try_write_packet(first_byte_num, audio_data, buf, abs_head);
        }

        if (!first_print || (first_print && (first_byte_num + buf.psize() - 1) >= (byte0 + params->bsize / 4 * 3))) {
            first_print = false;
            printer_state.store(printer_state_t::WORK);
            printer_cv.notify_one();
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
