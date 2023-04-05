#include "../common/err.h"
#include "../common/buffer.h"
#include "../common/endian.h"
#include "../common/audio_packet.h"

#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdarg>
#include <csignal>

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#define NO_SESSION 0

namespace bpo = boost::program_options;

int socket_fd;

struct receiver_params {
    std::string dest_addr;
    uint16_t    data_port;
    size_t      psize;
    size_t      bsize;
};

receiver_params get_params(int argc, char* argv[]) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",  "produce help message")
        ("addr,a",  bpo::value<std::string>(), "DEST_ADDR") // TODO: why the hell is this necessary
        ("port,P",  bpo::value<uint16_t>()->default_value(29629), "DATA_PORT")
        ("psize,p", bpo::value<size_t>()->default_value(512), "PSIZE")
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

    return receiver_params {
        .data_port = vm["port"].as<uint16_t>(),
        .psize     = vm["psize"].as<size_t>(),
        .bsize     = vm["bsize"].as<size_t>(),
    };

    // return receiver_params {
    //     .data_port = 29629,
    //     .psize     = 512,
    //     .bsize     = 65536,
    // };
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

static void read_packet(const int socket_fd, struct sockaddr_in* client_address, audio_packet* pkt, const size_t pkt_size) {
    socklen_t addr_len = sizeof(*client_address);
    ssize_t nread = recvfrom(socket_fd, pkt, pkt_size, 0, (struct sockaddr*)client_address, &addr_len);
    ENSURE((size_t)nread == pkt_size);
    pkt->first_byte_num = ntohll(pkt->first_byte_num);
    pkt->session_id = ntohll(pkt->session_id);
}

static uint64_t flush_threshold(const uint64_t byte0, const size_t bsize) {
    return byte0 + bsize * 3 / 4;
}

static void sigint_handler(int signum) {
    eprintln("Received SIGINT, receiver shutting down...");
    CHECK_ERRNO(close(socket_fd));
    exit(signum);
}

static void run(const receiver_params* params) {
    const size_t pkt_size = 2 * sizeof(uint64_t) + params->psize;
    uint8_t buffer[params->bsize];
    uint64_t byte0 = 0, cur_session = NO_SESSION;
    size_t nread = 0;
    bool reset = true;

    socket_fd = bind_socket(params->data_port);
    eprintln("Listening on port %u...", params->data_port);
    signal(SIGINT, sigint_handler);

    std::mutex mtx;
    std::condition_variable cv;
    bool ongoing_flush = false;

    for (;;) {
        struct sockaddr_in client_address;
        audio_packet pkt;
        read_packet(socket_fd, &client_address, &pkt, pkt_size);

        if (pkt.session_id < cur_session) {
            eprintln("Ignoring old session %llu...", pkt.session_id);
            continue;
        }

        if (reset || (pkt.session_id > cur_session)) {
            cur_session = pkt.session_id;
            byte0 = pkt.first_byte_num;
            nread = 0;
            reset = false;
        }
        
        size_t fst_pkt = byte0 / params->psize;
        size_t lst_pkt = fst_pkt + nread;
        size_t cur_pkt = pkt.first_byte_num / params->psize;

        if (cur_pkt - lst_pkt > 1) {
            for (size_t i = lst_pkt + 1; i < cur_pkt; i++) {
                eprintln("MISSING: BEFORE %zu EXPECTED %zu", cur_pkt, i);
            }
            reset = true;
            continue;
        } 

        memcpy(buffer + nread * params->psize, pkt.audio_data, params->psize);
        nread++;

        if (pkt.first_byte_num + params->psize - 1 >= flush_threshold(byte0, params->bsize)) {
            // Jeśli to odkomentujemy, to będzie wielowątkowo:

            // eprintln("Attempting to flush...");
            // std::unique_lock<std::mutex> lock(mtx);
            // cv.wait(lock, [&] { return !ongoing_flush; });
            // ongoing_flush = true;
            // eprintln("Flushing!");
            // auto t = std::thread([&] {
    
            size_t nflushed = write(STDOUT_FILENO, buffer, nread * params->psize);
            ENSURE(nflushed == nread * params->psize);
            
            //     std::unique_lock<std::mutex> lock(mtx);
            //     ongoing_flush = false;
            // });
            // t.detach();
            reset = true;
        }
    }
}

int main(int argc, char* argv[]) {
    receiver_params params = get_params(argc, argv);
    run(&params);

    return 0;
}
