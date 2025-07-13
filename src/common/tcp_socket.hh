#pragma once

#include "err.h"

#include <netinet/in.h>

#include <memory>
#include <sstream>
#include <string>

/**
 * @class TcpClientSocket
 * @brief Represents a TCP client socket.
 *
 * Encapsulates a connected TCP socket, providing methods for reading and
 * writing data over an established connection.
 */
struct TcpClientSocket {
public:
    /**
     * @brief Constructs a TcpClientSocket from an existing socket file descriptor.
     * @param fd The file descriptor of an already established TCP connection.
     */
    explicit TcpClientSocket(int fd);

    /**
     * @brief Destructor that closes the socket if necessary.
     */
    ~TcpClientSocket();

    /**
     * @brief Move assignment operator.
     * @param other The `TcpClientSocket` instance to move from.
     * @return Reference to the updated `TcpClientSocket` instance.
     */
    TcpClientSocket& operator=(TcpClientSocket&& other);

    /**
     * @brief Returns the socket file descriptor.
     * @return The file descriptor of the TCP connection.
     */
    int fd() const;

    /**
     * @brief Reads data from the socket.
     * @param buf Pointer to the buffer to store received data.
     * @param nbytes Maximum number of bytes to read.
     * @return True if the read was successful, false otherwise.
     */
    bool read(void* buf, size_t nbytes);

    /**
     * @brief Writes a message to the socket.
     * @param msg The message to send.
     */
    void write(const std::string& msg);

private:
    int _fd; ///< File descriptor for the TCP connection.
};

/**
 * @class TcpServerSocket
 * @brief Represents a TCP server socket.
 *
 * Provides functionality for creating a listening TCP socket that
 * can accept incoming client connections.
 */
struct TcpServerSocket {
private:
    static constexpr size_t DEFAULT_QUEUE_LEN = 42; ///< Default length of the connection queue.

    int _fd;           ///< File descriptor for the listening socket.
    in_port_t _port;   ///< Port number the server is bound to.
    size_t _queue_len; ///< Maximum length of the pending connection queue.

public:
    TcpServerSocket() = delete; // TODO: needed?

    /**
     * @brief Constructs a TcpServerSocket bound to a specific port.
     * @param port The port number to bind the server socket to.
     * @param queue_len The maximum length of the pending connection queue (default: `DEFAULT_QUEUE_LEN`).
     */
    explicit TcpServerSocket(in_port_t port, size_t queue_len = DEFAULT_QUEUE_LEN);

    /**
     * @brief Destructor that closes the listening socket.
     */
    ~TcpServerSocket();

    /**
     * @brief Puts the server socket into a listening state.
     *
     * Must be called before accepting connections.
     */
    void listen();

    /**
     * @brief Accepts an incoming client connection.
     * @return A `TcpClientSocket` representing the connected client.
     */
    TcpClientSocket accept();

    /**
     * @brief Returns the port the server socket is bound to.
     * @return The port number.
     */
    in_port_t port() const;

    /**
     * @brief Returns the file descriptor of the server socket.
     * @return The file descriptor.
     */
    int fd() const;
};
