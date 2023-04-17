#include "../common/err.h"
#include "../common/buffer.h"
#include "../common/endian.h"
#include "../common/audio_packet.h"

#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <csignal>

#include <iostream>
#include <string>
#ifdef BOOST
#include <boost/program_options.hpp>
#endif

#ifdef BOOST
namespace bpo = boost::program_options;
#endif

int socket_fd;

struct sender_params {
    uint64_t    session_id;
    std::string nazwa;
    std::string dest_addr;
    uint16_t    data_port;
    size_t      psize;
};

static ssize_t readn_blocking(uint8_t* buf, const size_t n) {
    uint8_t* bpos = buf;
    size_t nleft  = n;

    while (nleft) {
        ssize_t res = read(STDIN_FILENO, bpos, nleft);
        if (res == -1) {
            // TODO: is this needed?
            // if (errno == EINTR) {
            //     continue;
            // }
            return -1;
        } else if (res == 0) {
            break;
        }
        nleft -= res;
        bpos  += res;
    }
    return n - nleft;
}

static struct sockaddr_in get_send_address(const char* host, const uint16_t port) {
    struct addrinfo hints = {};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *address_result;
    CHECK(getaddrinfo(host, NULL, &hints, &address_result));

    struct sockaddr_in send_address;
    send_address.sin_family = AF_INET; // IPv4
    send_address.sin_addr.s_addr = ((struct sockaddr_in *) (address_result->ai_addr))->sin_addr.s_addr; // IP address
    send_address.sin_port = htons(port);

    freeaddrinfo(address_result);
    return send_address;
}

static void send_packet(const int socket_fd, uint64_t session_id, uint64_t first_byte_num, const uint8_t* audio_data, const size_t psize) {
    session_id = htonll(session_id);
    first_byte_num = htonll(first_byte_num);

    uint8_t buffer[2 * sizeof(uint64_t) + psize];
    uint8_t* bufpos = buffer;
    bufpos = bufcpy(bufpos, &session_id, sizeof(uint64_t));
    bufpos = bufcpy(bufpos, &first_byte_num, sizeof(uint64_t));
    bufcpy(bufpos, audio_data, psize);

    ssize_t sent_length = write(socket_fd, buffer, sizeof(buffer));
    ENSURE(sent_length == (ssize_t)sizeof(buffer));
}

struct sender_params get_params(int argc, char* argv[]) {
#ifdef BOOST
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
#endif
#ifndef BOOST
    return sender_params {
        .session_id = (uint64_t)time(NULL),
        .nazwa      = std::string("Nienazwany nadajnik"),
        .dest_addr  = std::string("localhost"),
        .data_port  = 29629,
        .psize      = 512,
    };
#endif
}

static void sigint_handler(int signum) {
    eprintln("Received SIGINT, receiver shutting down...");
    CHECK_ERRNO(close(socket_fd));
    exit(signum);
}

static void run(const sender_params* params) {
    struct sockaddr_in send_address = get_send_address(params->dest_addr.c_str(), params->data_port);
    int socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
    VERIFY(socket_fd);
    signal(SIGINT, sigint_handler);
    VERIFY(connect(socket_fd, (struct sockaddr *)&send_address, sizeof(send_address)));

    size_t nsent = 0;
    uint8_t audio_data[params->psize];

    for (size_t first_byte_num = 0; ; first_byte_num += params->psize) {
        ssize_t nread = readn_blocking(audio_data, params->psize);
        fprintf(stderr, "Read %zu bytes!\n", nread);

        VERIFY(nread);
        if (nread < (ssize_t)params->psize) {
            // End of input or incomplete packet
            break;
        }

        fprintf(stderr, "Sending packet %zu...\n", first_byte_num / params->psize);
        send_packet(socket_fd, params->session_id, first_byte_num, audio_data, params->psize);
        nsent += params->psize;
    }

    fprintf(stderr, "All done, sent %zu bytes!\n", nsent);
    CHECK_ERRNO(close(socket_fd));
}

int main(int argc, char* argv[]) {
    sender_params params = get_params(argc, argv);
    run(&params);
   
    return 0;
}
