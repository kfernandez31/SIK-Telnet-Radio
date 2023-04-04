#include "sender.hh"
#include "../common/err.h"

#include <boost/program_options.hpp>
#include <arpa/inet.h>
#include <time.h>
#include <iostream>
#include <string>

namespace bpo = boost::program_options;

// Pole session_id jest stałe przez cały czas uruchomienia nadajnika. 
// Na początku jego działania inicjowane jest datą wyrażoną w sekundach od początku epoki.

int main(int argc, char** argv) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("addr,a", bpo::value<std::string>(), "DEST_ADDR")
        ("port,P", bpo::value<uint16_t>()->default_value(29629), "DATA_PORT")
        ("psize,p", bpo::value<size_t>()->default_value(512), "PSIZE")
        ("bsize,b", bpo::value<size_t>()->default_value(65536), "BSIZE")
        ("nazwa,n", bpo::value<std::string>()->default_value("Nienazwany Nadajnik"), "NAZWA");

    bpo::variables_map vm;
    bpo::store(
        bpo::command_line_parser(argc, argv)
            .options(desc)
            .style(bpo::command_line_style::unix_style | bpo::command_line_style::allow_long_disguise)
            .run()
        , vm 
    );
    bpo::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }
    
    if (!vm.count("addr")) {
        fatal("DEST_ADDR is required.");
    }

    uint64_t session_id = (uint64_t)time(NULL);
    std::string dest_addr = vm["addr"].as<std::string>();
    uint16_t data_port = vm["port"].as<uint16_t>();
    size_t psize = vm["psize"].as<size_t>();
    size_t bsize = vm["bsize"].as<size_t>();
    std::string nazwa = vm["nazwa"].as<std::string>();

    struct sockaddr_in send_address = get_send_address(dest_addr.c_str(), data_port);
    int socket_fd = socket(PF_INET, SOCK_DGRAM, 0);
    VERIFY(socket_fd);
    VERIFY(connect(socket_fd, (struct sockaddr *)&send_address, (socklen_t)sizeof(send_address)));

    uint8_t buffer[bsize];
    for (;;) {
        ssize_t nread = read(STDIN_FILENO, (void*)&buffer, bsize);
        VERIFY(nread);

        // End of input or incomplete packet
        if (nread == 0 || nread % psize != 0) {
            break;
        }

        for (size_t i = 0; i < nread; i += psize) {
            send_packet(socket_fd, session_id, (uint64_t)i, buffer + i, psize);
        }
    }

    CHECK_ERRNO(close(socket_fd));
    return 0;
}
