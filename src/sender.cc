#include "utils/err.hh"
#include "utils/endian.hh"
#include "utils/audio_packet.hh"
#include "utils/net.hh"
#include "utils/mem.hh"

#include <unistd.h>
#include <csignal>
#include <poll.h>
#include <ctime>

#include <iostream>
#include <string>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

int socket_fd;

struct sender_params {
    uint64_t    session_id;
    std::string nazwa;
    std::string dest_addr;
    uint16_t    data_port;
    size_t      psize;
};

static size_t readn_blocking(uint8_t* buf, const size_t n) {
    uint8_t* bpos  = buf;
    size_t   nleft = n;
    while (nleft) {
        ssize_t res = read(STDIN_FILENO, bpos, nleft);
        VERIFY(res);
        if (res == 0)
            break;
        nleft -= res;
        bpos  += res;
    }
    return n - nleft;
}

static void send_packet(struct sockaddr_in* dst_addr, const int socket_fd, const uint64_t session_id, const uint64_t first_byte_num, const uint8_t* audio_data, const size_t psize) {
    uint8_t buffer[2 * sizeof(uint64_t) + psize];
    uint8_t* bufpos = buffer;
    bufpos = my_memcpy(bufpos, &session_id, sizeof(uint64_t));
    bufpos = my_memcpy(bufpos, &first_byte_num, sizeof(uint64_t));
    my_memcpy(bufpos, audio_data, psize);

    VERIFY(sendto(socket_fd, buffer, sizeof(buffer), 0, (sockaddr*)dst_addr, sizeof(*dst_addr)));
}

static sender_params get_params(int argc, char* argv[]) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",  "produce help message")
        ("addr,a",  bpo::value<std::string>(), "DEST_ADDR")
        ("port,P",  bpo::value<uint16_t>()->default_value(29629), "DATA_PORT")
        ("psize,p", bpo::value<size_t>()->default_value(512), "PSIZE")
        ("nazwa,n", bpo::value<std::string>()->default_value("Nienazwany Nadajnik"), "NAZWA");

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
        fatal("DEST_ADDR is required.");

    return sender_params {
        .session_id = (uint64_t)time(NULL),
        .nazwa      = vm["nazwa"].as<std::string>(),
        .dest_addr  = vm["addr"].as<std::string>(),
        .data_port  = vm["port"].as<uint16_t>(),
        .psize      = vm["psize"].as<size_t>(),
    };
}

static void cleanup() {
    CHECK_ERRNO(close(socket_fd));
}

static void signal_handler(int signum) {
    eprintln("Received %s. Shutting down...", strsignal(signum));
    cleanup();
    exit(signum);
}

static void run(const sender_params* params) {
    uint8_t audio_data[params->psize];
    struct sockaddr_in dst_addr = get_addr(params->dest_addr.c_str(), params->data_port);
    VERIFY(socket_fd = socket(PF_INET, SOCK_DGRAM, 0));
    signal(SIGINT, signal_handler);

    for (size_t nsent = 0;; nsent += params->psize) {
        size_t nread = readn_blocking(audio_data, params->psize);
        if (nread < params->psize)
            break; // End of input or incomplete packet
        send_packet(&dst_addr, socket_fd, htonll(params->session_id), htonll(nsent), audio_data, params->psize);
    }

    cleanup();
}

int main(int argc, char* argv[]) {
    sender_params params = get_params(argc, argv);
    run(&params);
    return 0;
}
