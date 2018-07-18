#ifndef BM_DATASTREAM_H
#define BM_DATASTREAM_H

#include <inttypes.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>

/**
 * A generic data stream.
 */
struct bm_datastream_s {
   /* Destroys the datastream */
   void (*destroy)(void*);
   /* Connect to stream; return 0 on error */
   int (*connect)(void*);
   /* Disconnects the stream */
   void (*disconnect)(void*);
   /* Send data on this stream; return bytes sent or <0 for error */
   ssize_t (*send)(void*, const uint8_t*, size_t);
   /* Receive data on this stream; return bytes received or <0 for error */
   ssize_t (*recv)(void*, uint8_t*, size_t);
   /* Stream status */
   enum {
      BM_DATASTREAM_UNKNOWN = 0,
      BM_DATASTREAM_READY,
      BM_DATASTREAM_ERROR
   } status;
   /* "unknown", "ready", or error message */
   char* status_desc;
   /* Stream descriptor */
   char* descriptor;
   /* Datastream id */
   char* id;
   /* The thread handle associated to this stream */
   pthread_t thread;
   /* Verbose flag */
   int verbose;
   /* Used to have manage the linked list of streams */
   struct bm_datastream_s* next;
};
typedef struct bm_datastream_s* bm_datastream_t;

/*
 * Initializes a generic datastream.
 * This function sets the initial status, the id, and the basic
 * methods of the stream.
 * This function must be called by any bm_*_datastream_new() function.
 * @param ds The datastream.
 * @param desc The stream descriptor.
 * @param destroyf The destroy() method.
 * @param connectf The connect() method.
 * @param disconnectf The disconnect() method.
 * @param sendf The send() method.
 * @param recvf The recv() method.
 */
extern void bm_datastream_init(bm_datastream_t ds,
                               const char* desc,
                               void (*destroyf)(void*),
                               int (*connectf)(void*),
                               void (*disconnectf)(void*),
                               ssize_t (*sendf)(void*, const uint8_t*, size_t),
                               ssize_t (*recvf)(void*, uint8_t*, size_t));

/*
 * Performs generic stream cleanup.
 * - Calls disconnect()
 * - Frees the strings
 * @param ds The datastream.
 */
extern void bm_datastream_destroy(bm_datastream_t ds);

/*
 * Sets the data stream status.
 * @param ds The datastream.
 * @param status The status code (BM_DATASTREAM_UNKNOWN, BM_DATASTREAM_READY, BM_DATASTREAM_ERROR)
 * @param desc The string description ("unknown", "ready", or error message)
 */
extern void bm_datastream_set_status(void* ds,
                                     int status,
                                     const char* desc,
                                     ...);

#endif
