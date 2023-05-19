#include "common/err.hh"
#include "common/endian.hh"
#include "common/audio_packet.hh"
#include "common/net.hh"
#include "common/mem.hh"
#include "common/circular_buffer.hh"
#include "common/protocol.hh"
#include "common/udp_socket.hh"
#include "common/radio_station.hh"

#include <netinet/in.h>
#include <unistd.h>
#include <csignal>

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <optional>
#include <set>

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

#define NO_SESSION                     0
#define MAX_UINT64_T_DIGITS            20
#define LOG_MSG_LEN                    (sizeof("MISSING: BEFORE  EXPECTED  \n") - 1)
#define TOTAL_LOG_LEN                  (2 * MAX_UINT64_T_DIGITS + LOG_MSG_LEN)
#define LOOKUP_REQUEST                 "ZERO_SEVEN_COME_IN"

struct receiver_params {
    std::optional<std::string> prio_station_name;
    in_port_t ctrl_port, ui_port;
    std::string discover_addr;
    size_t bsize;
};

int socket_fd;
bool printer_wait = true;
std::condition_variable printer_cv;
std::thread printer;
CircularBuffer* cb;
std::mutex cb_mtx;
volatile sig_atomic_t running = true;

static receiver_params get_params(int argc, char* argv[]) {
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help,h",          "produce help message")
        ("name,n",          bpo::value<std::string>(), "NAME")
        ("discover_addr,d", bpo::value<std::string>()->default_value("255.255.255.255"), "DISCOVER_ADDR")
        ("ctrl_port,C",     bpo::value<in_port_t>()->default_value(39629), "CTRL_PORT")
        ("ui_port,U",       bpo::value<in_port_t>()->default_value(19629), "UI_PORT")
        ("bsize,b",         bpo::value<size_t>()->default_value(65536), "BSIZE");

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

    if (!vm.count("mcast_addr"))
        fatal("MCAST_ADDR is required");

    receiver_params params;
    try {
        if (vm.count("name"))
            params.prio_station_name = vm["name"].as<std::string>();
        else
            params.prio_station_name = {};
        params.discover_addr     = vm["discover_addr"].as<std::string>();
        params.ctrl_port         = vm["ctrl_port"].as<in_port_t>();
        params.ui_port           = vm["ui_port"].as<in_port_t>();
        params.bsize             = vm["bsize"].as<size_t>();
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n' << desc;
        exit(EXIT_FAILURE);
    }
    if (params.bsize < 1)
        fatal("BSIZE must be positive");
    if (!RadioStation::is_valid_name(params.prio_station_name))
        fatal("invalid NAME");
    if (!is_valid_bcast_addr(params.discover_addr))
        fatal("invalid DISCOVER_ADDR");
    return params;
}

static void cleanup() {
    std::unique_lock<std::mutex> lock(cb_mtx);
    running = false;
    printer_wait = false;
    printer_cv.notify_one();
    printer.join();
    if (cb) {
        cb->~CircularBuffer();
        delete cb;
    }
    if (-1 == close(socket_fd))
        fatal("close");
}

static void signal_handler(int signum) {
    logerr("Received %s. Shutting down...", strsignal(signum));
    cleanup();
    exit(signum);
}

int main(int argc, char* argv[]) {
    receiver_params params = get_params(argc, argv);
    run(&params);
    return 0;
}
