cmake_minimum_required(VERSION 3.5)

# Set project name and language
project(sikradio C CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)

# Enable all warnings
add_compile_options(-Wall)

# Enable optimization level 2
add_compile_options(-O2)

# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS program_options)

# Add common library
add_library(common common/err.c)

# Add executable for receiver
add_executable(sikradio-receiver 
    receiver/main.cc 
    receiver/receiver.cc 
    receiver/receiver.hh
)
target_compile_options(sikradio-receiver PRIVATE -Wall -O2)
target_link_libraries(sikradio-receiver common Boost::program_options)

# Add executable for sender
add_executable(sikradio-sender 
    sender/main.cc 
    sender/sender.cc
    sender/sender.hh
)
target_compile_options(sikradio-sender PRIVATE -Wall -O2)
target_link_libraries(sikradio-sender common Boost::program_options)
