#include "utils/err.hh"
#include "utils/buffer.hh"
#include "utils/endian.hh"
#include "utils/audio_packet.hh"
#include "utils/net.hh"

#include <poll.h>
#include <unistd.h>
#include <ctime>
#include <csignal>

#include <iostream>
#include <string>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

int exit_code;
bool running = true;

struct sender_params {
    uint64_t    session_id;
    std::string nazwa;
    std::string dest_addr;
    uint16_t    data_port;
    size_t      psize;
};

static ssize_t readn_blocking(uint8_t* buf, const size_t n) {
    uint8_t* bpos  = buf;
    size_t   nleft = n;

    while (nleft) {
        ssize_t res = read(STDIN_FILENO, bpos, nleft);
        if (res == -1 && errno == EINTR)
            break;
        VERIFY(res);
        if (res == 0)
            break;
        nleft -= res;
        bpos  += res;
    }
    return n - nleft;
}

static void send_packet(const int socket_fd, uint64_t session_id, uint64_t first_byte_num, const uint8_t* audio_data, const size_t psize) {
    session_id     = htonll(session_id);
    first_byte_num = htonll(first_byte_num);

    uint8_t buffer[2 * sizeof(uint64_t) + psize];
    uint8_t* bufpos = buffer;
    bufpos = bufcpy(bufpos, &session_id, sizeof(uint64_t));
    bufpos = bufcpy(bufpos, &first_byte_num, sizeof(uint64_t));
    bufcpy(bufpos, audio_data, psize);

    ssize_t sent_length = write(socket_fd, buffer, sizeof(buffer));
    ENSURE(sent_length == (ssize_t)sizeof(buffer));
}

static sender_params get_params(int argc, char* argv[]) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("addr,a",  bpo::value<std::string>(), "DEST_ADDR")
        ("port,P",  bpo::value<uint16_t>()->default_value(29629), "DATA_PORT")
        ("psize,p", bpo::value<size_t>()->default_value(512), "PSIZE")
        ("nazwa,n", bpo::value<std::string>()->default_value("Nienazwany Nadajnik"), "NAZWA");

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

    return sender_params {
        .session_id = (uint64_t)time(NULL),
        .nazwa      = vm["nazwa"].as<std::string>(),
        .dest_addr  = vm["addr"].as<std::string>(),
        .data_port  = vm["port"].as<uint16_t>(),
        .psize      = vm["psize"].as<size_t>(),
    };
}

static void signal_handler(int signum) {
    eprintln("Received %s. Shutting down...", strsignal(signum));
    running = false;
    exit_code = signum;
}

static int run(const sender_params* params) {
    uint8_t audio_data[params->psize];
    struct sockaddr_in dst_addr = get_addr(params->dest_addr.c_str(), params->data_port);
    int socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
    VERIFY(socket_fd);
    VERIFY(connect(socket_fd, (struct sockaddr *)&dst_addr, sizeof(dst_addr)));
    
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    VERIFY(sigaction(SIGINT, &sa, NULL));

    size_t nsent = 0;
    while (running) {
        ssize_t nread = readn_blocking(audio_data, params->psize);
        eprintln("Read %zu bytes!", nread);

        VERIFY(nread);
        if (nread < (ssize_t)params->psize)
            break; // End of input or incomplete packet

        eprintln("Sending packet %zu...\n", nsent);
        send_packet(socket_fd, params->session_id, nsent, audio_data, params->psize);
        nsent += params->psize;
    }

    eprintln("All done, sent %zu bytes!\n", nsent);
    CHECK_ERRNO(close(socket_fd));
    return exit_code;
}

int main(int argc, char* argv[]) {
    sender_params params = get_params(argc, argv);
    return run(&params);
}
