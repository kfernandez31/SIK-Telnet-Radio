# Set compiler and flags
CC = g++
CFLAGS = -Wall -O2 -std=c++20
# LDFLAGS := -pthread
LDFLAGS := -lboost_program_options
INCLUDES = -I/usr/include/boost

# Set source directories
COMMON_DIR = common
RECEIVER_DIR = receiver
SENDER_DIR = sender

# Object files
COMMON_OBJS = $(COMMON_DIR)/buffer.o $(COMMON_DIR)/err.o
RECEIVER_OBJS = $(RECEIVER_DIR)/receiver.o $(COMMON_OBJS)
SENDER_OBJS = $(SENDER_DIR)/sender.o $(COMMON_OBJS)

# Binary files
RECEIVER_BIN = sikradio-receiver
SENDER_BIN = sikradio-sender

# Targets
.PHONY: all clean

all: $(RECEIVER_BIN) $(SENDER_BIN)

$(RECEIVER_BIN): $(RECEIVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SENDER_BIN): $(SENDER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(COMMON_DIR)/%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(RECEIVER_DIR)/%.o: $(RECEIVER_DIR)/%.cc
	$(CC) $(CFLAGS) -c -o $@ $<

$(SENDER_DIR)/%.o: $(SENDER_DIR)/%.cc
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(RECEIVER_BIN) $(SENDER_BIN) $(COMMON_DIR)/*.o $(RECEIVER_DIR)/*.o $(SENDER_DIR)/*.o
