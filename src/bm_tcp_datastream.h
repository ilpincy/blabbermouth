#ifndef BM_TCP_DATASTREAM_H
#define BM_TCP_DATASTREAM_H

#include "bm_datastream.h"

/*
 * The string for tcp connect is:
 * tcp:server:port
 */

struct bm_tcp_datastream_s {
   /* Generic datastream definition */
   struct bm_datastream_s parent;
   /* Socket stream */
   int stream;
   /* Server */
   char* server;
   /* Port */
   char* port;
};
typedef struct bm_tcp_datastream_s* bm_tcp_datastream_t;

/*
 * Creates a new TCP datastream.
 * @param desc The stream descriptor.
 * @return The new TCP datastream.
 */
extern bm_tcp_datastream_t bm_tcp_datastream_new(const char* desc);

#endif
