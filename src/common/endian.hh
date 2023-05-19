#pragma once

#include <arpa/inet.h>

#ifndef htonll
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define htonll(x) (((uint64_t)htonl(x) << 32) | htonl(x >> 32))
    #else
        #define htonll(x) (x)
    #endif
#endif

#ifndef ntohll
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define ntohll(x) (((uint64_t)ntohl(x) << 32) | ntohl(x >> 32))
    #else
        #define ntohll(x) (x)
    #endif
#endif
