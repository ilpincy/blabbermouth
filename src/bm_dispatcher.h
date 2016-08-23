#ifndef BM_DISPATCHER_H
#define BM_DISPATCHER_H

#include "bm_datastream.h"

/*
 * The dispatcher state.
 */
struct bm_dispatcher_s {
   /* A linked list of streams */
   bm_datastream_t streams;
   /* The number of streams */
   size_t stream_num;
   /* The message length */
   size_t msg_len;
   /* PThread condition variable to start the streams */
   pthread_cond_t startcond;
   /* PThread mutex to start the streams */
   pthread_mutex_t startmutex;
   /* A condition variable to start the streams */
   int start;
   /* PThread mutex to send data */
   pthread_mutex_t datamutex;
};
typedef struct bm_dispatcher_s* bm_dispatcher_t;

/*
 * Creates a new dispatcher.
 * @return A new dispatcher instance.
 */
extern bm_dispatcher_t bm_dispatcher_new();

/*
 * Destroys the dispatcher.
 * @param d The dispatcher
 */
extern void bm_dispatcher_destroy(bm_dispatcher_t d);

/*
 * Adds a stream to the dispatcher.
 * @param d The dispatcher
 * @param s The stream descriptor
 * @return 1 for success, 0 for failure.
 */
extern int bm_dispatcher_stream_add(bm_dispatcher_t d,
                                    const char* s);

/*
 * Executes the dispatcher.
 * @param d The dispatcher
 */
extern void bm_dispatcher_execute(bm_dispatcher_t d);

#endif
