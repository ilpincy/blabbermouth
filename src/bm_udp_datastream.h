#ifndef BM_UDP_DATASTREAM_H
#define BM_UDP_DATASTREAM_H

#include "bm_datastream.h"

/*
 * The string for udp connect is:
 * udp:server:port
 */

struct bm_udp_datastream_s {
   /* Generic datastream definition */
   struct bm_datastream_s parent;
   /* Socket stream */
   int stream;
   /* Server */
   char* server;
   /* Port */
   char* port;
};
typedef struct bm_udp_datastream_s* bm_udp_datastream_t;

/*
 * Creates a new UDP datastream.
 * @param desc The stream descriptor.
 * @return The new UDP datastream.
 */
extern bm_udp_datastream_t bm_udp_datastream_new(const char* desc);

#endif
