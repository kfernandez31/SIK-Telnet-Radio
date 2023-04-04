#include "receiver.hh"
#include "../common/err.h"

#include <boost/program_options.hpp>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <thread>

const uint64_t NO_SESSION = 0;

namespace bpo = boost::program_options;

int main(int argc, char** argv) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("addr,a", bpo::value<std::string>(), "DEST_ADDR")
        ("port,P", bpo::value<uint16_t>()->default_value(29629), "DATA_PORT")
        ("psize,p", bpo::value<size_t>()->default_value(512), "PSIZE")
        ("bsize,b", bpo::value<size_t>()->default_value(65536), "BSIZE");

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
        return 1;
    }

    std::string dest_addr = vm["addr"].as<std::string>();
    uint16_t data_port = vm["port"].as<uint16_t>();
    size_t psize = vm["psize"].as<size_t>();
    size_t bsize = vm["bsize"].as<size_t>();

    fprintf(stderr, "Listening on port %u...\n", data_port);

    const size_t pkt_size = 2 * sizeof(uint64_t) + psize;

    uint64_t byte0, cur_session = NO_SESSION;
    size_t highest_byte_received = 0, dump_pos = 0;
    int socket_fd = bind_socket(data_port);
    uint8_t buffer[bsize];
    bool received[bsize / psize];
    for (int i = 0; i < bsize / psize; i++) {
        received[i] = false;
    }

    for (;;) {
        struct sockaddr_in client_address;
        audio_packet pkt;
        read_packet(socket_fd, &client_address, &pkt, pkt_size);

        // ignore older sessions
        if (pkt.session_id < cur_session) {
            continue;
        }

        // favor newer sessions
        if (pkt.session_id > cur_session) {
            // make this the current session
            cur_session = pkt.session_id;
            dump_pos = byte0 = pkt.first_byte_num;
            highest_byte_received = byte0;

            for (int i = 0; i < bsize / psize; i++) {
                received[i] = false;
            }
        } else {
            size_t first_packet = (size_t)byte0 / psize;
            size_t this_packet = (size_t)pkt.first_byte_num / psize;
            for (size_t i = first_packet; i < this_packet; i++) {
                if (!received[i]) {
                    fprintf(stderr, "MISSING: BEFORE %zu EXPECTED %zu\n", this_packet, i);
                }
            }
        }

        memcpy(buffer + pkt.first_byte_num, pkt.audio_data, psize);
        received[pkt.first_byte_num / psize] = true;

        if (highest_byte_received >= write_threshold(byte0, bsize)) {
            dump_buffer(buffer + dump_pos, highest_byte_received - dump_pos + 1);
            dump_pos = highest_byte_received;
        }
    }

    CHECK_ERRNO(close(socket_fd));
    return 0;
}
