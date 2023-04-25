cmake_minimum_required(VERSION 3.5)

# Set project name and language
project(sikradio)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "-Wall -O2")

# Find Boost libraries
find_package(Boost REQUIRED COMPONENTS program_options REQUIRED)

# Add executable for receiver
add_executable(sikradio-receiver src/receiver.cc src/utils/audio_packet.cc src/utils/cyclical_buffer.cc src/utils/err.cc src/utils/mem.cc src/utils/net.cc)
target_link_libraries(sikradio-receiver Boost::program_options)

# Add executable for sender
add_executable(sikradio-sender src/sender.cc src/utils/audio_packet.cc src/utils/cyclical_buffer.cc src/utils/err.cc src/utils/mem.cc src/utils/net.cc)
target_link_libraries(sikradio-sender Boost::program_options)