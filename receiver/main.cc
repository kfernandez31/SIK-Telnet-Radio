#include "receiver.hh"
#include "../common/err.h"

#include <boost/program_options.hpp>

#include <iostream>
#include <string>

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
    std::string nazwa = vm["nazwa"].as<std::string>();

    return 0;
}
