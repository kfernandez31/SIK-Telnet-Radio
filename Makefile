CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2
LDFLAGS = -lboost_program_options -pthread
INCLUDES = -I/usr/include/boost

# Source files
RECEIVER_SRCS := \
    src/common/log.cc \
    src/common/net.cc \
    src/common/udp_socket.cc \
    src/common/tcp_socket.cc \
    src/common/radio_station.cc \
    src/common/datagram.cc \
    src/common/circular_buffer.cc \
    src/common/event_queue.cc \
    src/receiver/rexmit_sender.cc \
    src/receiver/audio_printer.cc \
    src/receiver/audio_receiver.cc \
    src/receiver/lookup_receiver.cc \
    src/receiver/lookup_sender.cc \
    src/receiver/station_remover.cc \
    src/receiver/ui_menu.cc \
    src/receiver/receiver.cc \

SENDER_SRCS := \
    src/common/log.cc \
    src/common/net.cc \
    src/common/udp_socket.cc \
    src/common/radio_station.cc \
    src/common/datagram.cc \
    src/common/circular_buffer.cc \
    src/common/event_queue.cc \
    src/sender/retransmitter.cc \
    src/sender/audio_sender.cc \
    src/sender/controller.cc \
    src/sender/sender.cc \

# Output executables
RECEIVER_TARGET := sikradio-receiver
SENDER_TARGET := sikradio-sender

all: sikradio-sender sikradio-receiver

sikradio-receiver:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(RECEIVER_SRCS) -o $(RECEIVER_TARGET) $(LDFLAGS)

sikradio-sender:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SENDER_SRCS) -o $(SENDER_TARGET) $(LDFLAGS)

%.o: %.cc
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(RECEIVER_TARGET) $(SENDER_TARGET)
