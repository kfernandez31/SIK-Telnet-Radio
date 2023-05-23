cmake_minimum_required(VERSION 3.5)

# Set project name and language
project(sikradio)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "-Wall -O2")

# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS program_options REQUIRED)

# # Add executable for receiver
# add_executable(sikradio-receiver 
#     src/common/udp_socket.cc
#     src/common/audio_packet.cc 
#     src/common/circular_buffer.cc
#     src/common/err.cc 
#     src/common/mem.cc 
#     src/common/net.cc 
#     src/receiver.cc
#     src/common/radio_station.cc
# )
# target_link_libraries(sikradio-receiver Boost::program_options)
# target_link_libraries(sikradio-receiver pthread)

# Add executable for sender
add_executable(sikradio-sender
    src/common/err.cc
    src/common/except.cc
    src/common/net.cc
    src/common/worker.cc
    src/common/udp_socket.cc
    src/common/datagram.cc
    src/common/circular_buffer.cc
    src/common/radio_station.cc
    src/sender/audio_sender.cc
    src/sender/retransmitter.cc
    src/sender/controller.cc
    src/sender/sender.cc
)
target_link_libraries(sikradio-sender Boost::program_options)
target_link_libraries(sikradio-sender pthread)
