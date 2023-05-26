#pragma once

#include "../common/err.hh"
#include "../common/net.hh"
#include "../common/except.hh"
#include "../common/datagram.hh"
#include "../common/radio_station.hh"

#include <netinet/in.h>
#include <cstddef>

#include <string>
#include <chrono>
#include <optional>
#include <iostream>

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

struct ReceiverParams {
    std::optional<std::string> prio_station_name;
    in_port_t ctrl_port, ui_port;
    std::string discover_addr;
    size_t bsize;
    std::chrono::milliseconds rtime;

    ReceiverParams() = default;

    ReceiverParams(int argc, char* argv[]) {
        bpo::options_description desc("Allowed options");
        desc.add_options()
            ("help,h",          "produce help message")
            ("name,n",          bpo::value<std::string>(), "NAME")
            ("discover_addr,d", bpo::value<std::string>()->default_value("255.255.255.255"), "DISCOVER_ADDR")
            ("ctrl_port,C",     bpo::value<in_port_t>()->default_value(39629), "CTRL_PORT")
            ("ui_port,U",       bpo::value<in_port_t>()->default_value(19629), "UI_PORT")
            ("bsize,b",         bpo::value<size_t>()->default_value(65536), "BSIZE")
            ("rtime,R",      bpo::value<size_t>()->default_value(250), "RTIME");

        bpo::variables_map vm;
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            std::cout << desc;
            exit(EXIT_SUCCESS);
        }

        if (!vm.count("name"))
            prio_station_name = {};
        else {
            prio_station_name = vm["name"].as<std::string>();
            if (!RadioStation::is_valid_name(*prio_station_name))
                throw RadioException("NAME is invalid");
        }
        discover_addr         = vm["discover_addr"].as<std::string>();
        ctrl_port             = vm["ctrl_port"].as<in_port_t>();
        ui_port               = vm["ui_port"].as<in_port_t>();
        bsize                 = vm["bsize"].as<size_t>();
        rtime                 = std::chrono::milliseconds(vm["rtime"].as<size_t>());

        if (bsize < 1)
            throw RadioException("BSIZE must be positive");
    }
};
