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
#include <iostream>

#include <boost/program_options.hpp>

namespace bpo = boost::program_options;

struct SenderParams {
    std::string name, mcast_addr;
    in_port_t data_port, ctrl_port;
    size_t psize, fsize;
    uint64_t session_id;
    std::chrono::milliseconds rtime;

    SenderParams() = default;

    SenderParams(int argc, char* argv[]) {
        bpo::options_description desc("Allowed options");
        desc.add_options()
            ("help,h",       "produce help message")
            ("name,n",       bpo::value<std::string>()->default_value("Nienazwany Nadajnik"), "NAME")
            ("mcast_addr,a", bpo::value<std::string>(), "MCAST_ADDR")
            ("data_port,P",  bpo::value<in_port_t>()->default_value(29629), "DATA_PORT")
            ("ctrl_port,C",  bpo::value<in_port_t>()->default_value(39629), "CTRL_PORT")
            ("psize,p",      bpo::value<size_t>()->default_value(512), "PSIZE")
            ("fsize,f",      bpo::value<size_t>()->default_value(131072), "FSIZE")
            ("rtime,R",      bpo::value<size_t>()->default_value(250), "RTIME");

        bpo::variables_map vm;
        bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
        bpo::notify(vm);

        if (vm.count("help")) {
            std::cout << desc;
            exit(EXIT_SUCCESS);
        }
        
        if (!vm.count("mcast_addr"))
            throw RadioException("MCAST_ADDR is required");

        session_id = (uint64_t)time(NULL);
        name       = vm["name"].as<std::string>();
        mcast_addr = vm["mcast_addr"].as<std::string>();
        data_port  = vm["data_port"].as<in_port_t>();
        ctrl_port  = vm["ctrl_port"].as<in_port_t>();
        psize      = vm["psize"].as<size_t>();
        fsize      = vm["fsize"].as<size_t>();
        rtime      = std::chrono::milliseconds(vm["rtime"].as<size_t>());

        if (psize < 1)
            throw RadioException("PSIZE must be positive");
        if (psize > AudioPacket::MAX_PSIZE)
            throw RadioException("PSIZE too big");
        if (fsize < 1)
            throw RadioException("FSIZE must be positive");
        if (!RadioStation::is_valid_name(name))
            throw RadioException("invalid NAME");
        if (!is_valid_mcast_addr(mcast_addr.c_str()))
            throw RadioException("invalid MCAST_ADDR");
    }
};
