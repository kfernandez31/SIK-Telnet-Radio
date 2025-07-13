#pragma once

#include "net.hh"
#include <netinet/in.h>

/**
 * @class UdpSocket
 * @brief A lightweight wrapper for a UDP socket.
 *
 * This class provides functionality for creating, binding, sending, and receiving
 * data over UDP sockets. It also supports multicast operations and various socket options.
 */
struct UdpSocket {
private:
    static constexpr int DEFAULT_TTL = 4; ///< Default multicast TTL.

    int _fd;                  ///< File descriptor for the socket.
    bool _mcast_recv_enabled; ///< Indicates if multicast reception is enabled.
    bool _connected;          ///< Indicates whether the socket is connected.
    ip_mreq_source _ipmreq;   ///< Struct for managing multicast group membership.

    /**
     * @brief Sets a socket option.
     * @param proto Protocol level (e.g., SOL_SOCKET, IPPROTO_IP).
     * @param type Option type (e.g., SO_BROADCAST, SO_REUSEADDR).
     * @param val Pointer to the option value.
     * @param val_size Size of the option value.
     */
    void set_opt(int proto, int type, void* val, size_t val_size);

    /**
     * @brief Binds the socket to a specific local port.
     * @param port Port number to bind to.
     */
    void set_local_port(in_port_t port);

public:
    sockaddr_in local_addr; ///< Stores the local address of the socket.
    sockaddr_in conn_addr;  ///< Stores the address of the connected peer.

    /**
     * @brief Constructs a UDP socket.
     *
     * Initializes the socket and binds it to an ephemeral port by default.
     * Terminates execution if the socket creation fails.
     */
    UdpSocket();

    /**
     * @brief Destructor that closes the socket and cleans up resources.
     */
    ~UdpSocket();

    /**
     * @brief Returns the socket file descriptor.
     * @return The file descriptor associated with the UDP socket.
     */
    int fd() const;

    /**
     * @brief Move assignment operator.
     *
     * Transfers ownership of the socket descriptor and internal state.
     * @param other Another `UdpSocket` object to move from.
     * @return Reference to the updated `UdpSocket`.
     */
    UdpSocket& operator=(UdpSocket&& other);

    /**
     * @brief Enables broadcasting on the socket.
     */
    void set_broadcast();

    /**
     * @brief Sets a receive timeout on the socket.
     * @param secs Timeout duration in seconds.
     */
    void set_receiving_timeout(int secs);

    /**
     * @brief Allows reusing the address for binding.
     */
    void set_reuseaddr();

    /**
     * @brief Sets the multicast TTL (Time-To-Live).
     * @param ttl The TTL value (default: `DEFAULT_TTL`).
     */
    void set_mcast_ttl(int ttl = DEFAULT_TTL);

    /**
     * @brief Sets a send timeout on the socket.
     * @param secs Timeout duration in seconds.
     */
    void set_sending_timeout(int secs);

    /**
     * @brief Adds the socket to a multicast group.
     */
    void set_add_membership();

    /**
     * @brief Removes the socket from a multicast group.
     */
    void set_drop_membership();

    /**
     * @brief Enables multicast reception from a specific source.
     * @param multicast_addr The multicast group address.
     * @param source_addr The specific source address for multicast reception.
     */
    void enable_mcast_recv(const sockaddr_in& multicast_addr, const sockaddr_in& source_addr);

    /**
     * @brief Disables multicast reception.
     */
    void disable_mcast_recv();

    /**
     * @brief Binds the socket to the specified port.
     * @param port Port number (default: 0, assigns an ephemeral port).
     */
    void bind(in_port_t port = 0);

    /**
     * @brief Connects the socket to a remote address.
     * @param conn_addr The address to connect to.
     */
    void connect(const sockaddr_in& conn_addr);

    /**
     * @brief Writes data to the socket (when connected).
     * @param buf Pointer to the data buffer.
     * @param nbytes Number of bytes to write.
     */
    void write(const void* buf, size_t nbytes) const;

    /**
     * @brief Reads data from the socket (when connected).
     * @param buf Pointer to the buffer to store received data.
     * @param nbytes Maximum number of bytes to read.
     * @return The number of bytes actually read.
     */
    size_t read(void* buf, size_t nbytes) const;

    /**
     * @brief Sends data to a specific destination.
     * @param buf Pointer to the data buffer.
     * @param nbytes Number of bytes to send.
     * @param dst_addr Destination address.
     * @return Number of bytes sent, or -1 on error.
     */
    ssize_t sendto(const void* buf, size_t nbytes, const sockaddr_in &dst_addr) const;

    /**
     * @brief Receives data and retrieves the sender's address.
     * @param buf Pointer to the buffer to store received data.
     * @param nbytes Maximum number of bytes to receive.
     * @param src_addr Structure to store sender's address.
     * @return Number of bytes received, or -1 on error.
     */
    ssize_t recvfrom(void* buf, size_t nbytes, sockaddr_in& src_addr) const;
};
