#pragma once

#include <arpa/inet.h>

/**
 * @def htonll(x)
 * @brief Converts a 64-bit integer from host byte order to network byte order.
 *
 * - On **little-endian** systems, it swaps the byte order manually using `htonl()`.
 * - On **big-endian** systems, it does nothing, as the byte order is already correct.
 *
 * @param x The 64-bit integer to convert.
 * @return The converted 64-bit integer in network byte order.
 */
#ifndef htonll
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define htonll(x) (((uint64_t)htonl((x)) << 32) | htonl((x) >> 32))
    #else
        #define htonll(x) (x)
    #endif
#endif

/**
 * @def ntohll(x)
 * @brief Converts a 64-bit integer from network byte order to host byte order.
 *
 * - On **little-endian** systems, it swaps the byte order manually using `ntohl()`.
 * - On **big-endian** systems, it does nothing, as the byte order is already correct.
 *
 * @param x The 64-bit integer to convert.
 * @return The converted 64-bit integer in host byte order.
 */
#ifndef ntohll
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define ntohll(x) (((uint64_t)ntohl((x)) << 32) | ntohl((x) >> 32))
    #else
        #define ntohll(x) (x)
    #endif
#endif
