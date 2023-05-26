CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2
LDFLAGS = -lboost_program_options -pthread
INCLUDES = -I/usr/include/boost

# Source files
RECEIVER_SRCS := src/common/err.cc \
                 src/common/except.cc \
                 src/common/net.cc \
                 src/common/worker.cc \
                 src/common/udp_socket.cc \
                 src/common/tcp_socket.cc \
                 src/common/datagram.cc \
                 src/common/circular_buffer.cc \
                 src/common/radio_station.cc \
                 src/common/event_pipe.cc \
                 src/receiver/rexmit_sender.cc \
                 src/receiver/audio_printer.cc \
                 src/receiver/audio_receiver.cc \
                 src/receiver/lookup_receiver.cc \
                 src/receiver/lookup_sender.cc \
                 src/receiver/station_remover.cc \
                 src/receiver/ui_menu.cc \
                 src/receiver/receiver.cc

SENDER_SRCS := src/common/err.cc \
               src/common/except.cc \
               src/common/net.cc \
               src/common/worker.cc \
               src/common/udp_socket.cc \
               src/common/datagram.cc \
               src/common/circular_buffer.cc \
               src/common/radio_station.cc \
               src/common/event_pipe.cc \
               src/sender/audio_sender.cc \
               src/sender/retransmitter.cc \
               src/sender/controller.cc \
               src/sender/sender.cc

# Output executables
RECEIVER_TARGET := sikradio-receiver
SENDER_TARGET := sikradio-sender

.PHONY: all receiver sender clean

all: sikradio-sender sikradio-receiver

receiver:
    $(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(RECEIVER_SRCS) -o $(RECEIVER_TARGET) $(LDFLAGS)

sender:
    $(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) $(SENDER_SRCS) -o $(SENDER_TARGET) $(LDFLAGS)

clean:
    rm -f $(RECEIVER_TARGET) $(SENDER_TARGET)

