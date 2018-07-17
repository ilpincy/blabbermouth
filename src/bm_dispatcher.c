#include "bm_dispatcher.h"
#include "bm_tcp_datastream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

/****************************************/
/****************************************/

/*
 * This is set to 0 at the beginning to mean: we're not done.
 * When set to 1, this means: the program must finish.
 * The value of this variable is set by a signal handler.
 */
static int done = 0;

/*
 * Active threads
 */
static int active_threads = 0;

/****************************************/
/****************************************/

void bm_dispatcher_broadcast(bm_dispatcher_t dispatcher,
                             bm_datastream_t stream,
                             const uint8_t* data) {
   pthread_mutex_lock(&dispatcher->datamutex);
   bm_datastream_t cur = dispatcher->streams;
   ssize_t sent;
   while(cur) {
      if(cur != stream)
         sent = cur->send(cur, data, dispatcher->msg_len);
      if(sent < dispatcher->msg_len) {
         fprintf(stderr, "sent %zd bytes instead of %zu to %s: %s\n",
                 sent,
                 dispatcher->msg_len,
                 cur->descriptor,
                 cur->status_desc);
      }
      cur = cur->next;
   }
   pthread_mutex_unlock(&dispatcher->datamutex);
}

/****************************************/
/****************************************/

struct bm_dispatcher_thread_data_s {
   bm_dispatcher_t dispatcher;
   bm_datastream_t stream;
};
typedef struct bm_dispatcher_thread_data_s* bm_dispatcher_thread_data_t;

void* bm_dispatcher_thread(void* arg) {
   /* Get thread data */
   bm_dispatcher_thread_data_t data = (bm_dispatcher_thread_data_t)arg;
   /* Wait for start signal */
   pthread_mutex_lock(&data->dispatcher->startmutex);
   ++active_threads;
   while(data->dispatcher->start == 0)
      pthread_cond_wait(&data->dispatcher->startcond,
                        &data->dispatcher->startmutex);
   pthread_mutex_unlock(&data->dispatcher->startmutex);
   /* Execute logic */
   uint8_t* buf = (uint8_t*)calloc(data->dispatcher->msg_len, 1);
   while(!done) {
      /* Receive data */
      if(data->stream->recv(data->stream,
                            buf,
                            data->dispatcher->msg_len) <= 0) {
         /* Error receiving data, exit */
         fprintf(stderr, "%s: exiting\n", data->stream->descriptor);
         pthread_mutex_lock(&data->dispatcher->startmutex);
         --active_threads;
         pthread_mutex_unlock(&data->dispatcher->startmutex);
         return NULL;
      }
      /* Broadcast data */
      bm_dispatcher_broadcast(data->dispatcher,
                              data->stream,
                              buf);
   }
   /* All done */
   pthread_mutex_lock(&data->dispatcher->startmutex);
   --active_threads;
   pthread_mutex_unlock(&data->dispatcher->startmutex);
   return NULL;
}

/****************************************/
/****************************************/

bm_dispatcher_t bm_dispatcher_new() {
   bm_dispatcher_t d = (bm_dispatcher_t)malloc(sizeof(struct bm_dispatcher_s));
   d->streams = NULL;
   d->stream_num = 0;
   d->start = 0;
   d->msg_len = 0;
   if(pthread_cond_init(&d->startcond, NULL) != 0) {
      fprintf(stderr, "Error initializing the start condition variable: %s\n",
              strerror(errno));
      free(d);
      return NULL;
   }
   if(pthread_mutex_init(&d->startmutex, NULL) != 0) {
      fprintf(stderr, "Error initializing the start mutex: %s\n",
              strerror(errno));
      free(d);
      return NULL;
   }
   if(pthread_mutex_init(&d->datamutex, NULL) != 0) {
      fprintf(stderr, "Error initializing the data mutex: %s\n",
              strerror(errno));
      free(d);
      return NULL;
   }
   return d;
}

/****************************************/
/****************************************/

void bm_dispatcher_destroy(bm_dispatcher_t d) {
   pthread_cond_destroy(&d->startcond);
   pthread_mutex_destroy(&d->startmutex);
   pthread_mutex_destroy(&d->datamutex);
   bm_datastream_t cur = d->streams;
   bm_datastream_t next;
   while(cur) {
      next = cur->next;
      cur->destroy(cur);
      cur = next;
   }
   free(d);
}

/****************************************/
/****************************************/

int bm_dispatcher_stream_add(bm_dispatcher_t d,
                             const char* s) {
   char* ws = strdup(s);
   char* saveptr;
   /* Get stream id */
   char* tok = strtok_r(ws, ":", &saveptr);
   if(!tok) {
      fprintf(stderr, "Can't parse '%s'\n", s);
      free(ws);
      return 0;
   }
   /* Make sure id has not been already used */
   for(bm_datastream_t cur = d->streams;
       cur != NULL;
       cur = cur->next) {
      if(strcmp(tok, cur->id) == 0) {
         fprintf(stderr, "'%s': id '%s' already in use by '%s'\n",
                 s,
                 tok,
                 cur->descriptor);
         free(ws);
         return 0;
      }
   }
   /* Get stream type */
   tok = strtok_r(NULL, ":", &saveptr);
   if(!tok) {
      fprintf(stderr, "Can't parse '%s'\n", s);
      free(ws);
      return 0;
   }
   /* Create the stream */
   bm_datastream_t stream;
   if(strcmp(tok, "tcp") == 0) {
      /* Create new TCP stream */
      stream = (bm_datastream_t)bm_tcp_datastream_new(s);
   }
#ifdef BLABBERMOUTH_WITH_BT
   else if(strcmp(tok, "bt") == 0) {
      /* Create new Bluetooth stream */
      fprintf(stderr, "'%s': Bluetooth streams not supported yet\n", s);
      free(ws);
      return 0;
   }
#endif
   /* else if(strcmp(tok, "xbee") == 0) { */
   /*    /\* Create new XBee stream *\/ */
   /*    fprintf(stderr, "'%s': XBee streams not supported yet\n", s); */
   /*    free(ws); */
   /*    return 0; */
   /* } */
   else {
      fprintf(stderr, "'%s': Unknown stream type '%s'\n", s, tok);
      free(ws);
      return 0;
   }
   /* Attempt to connect */
   if(!stream->connect(stream)) {
      fprintf(stderr, "'%s': Connection error: %s\n", s, stream->status_desc);
      bm_datastream_destroy(stream);
      free(ws);
      return 0;
   }
   /* Add a thread dedicated to this stream */
   bm_dispatcher_thread_data_t info =
      (bm_dispatcher_thread_data_t)malloc(
         sizeof(struct bm_dispatcher_thread_data_s));
   info->dispatcher = d;
   info->stream = stream;
   if(pthread_create(&stream->thread, NULL, &bm_dispatcher_thread, info) != 0) {
      fprintf(stderr, "'%s': Can't create thread: %s\n", s, strerror(errno));
      bm_datastream_destroy(stream);
      free(info);
      free(ws);
      return 0;
   }
   /* Add stream at the beginning of the list */
   if(d->streams != NULL)
      stream->next = d->streams;
   d->streams = stream;
   ++d->stream_num;
   /* Wrap up */
   fprintf(stdout, "Added stream '%s'\n", s);
   free(ws);
   return 1;
}

/****************************************/
/****************************************/

void sighandler(int sig) {
   /* The program is done */
   fprintf(stdout, "Termination requested\n");
   done = 1;
}

void bm_dispatcher_execute(bm_dispatcher_t d) {
   /* Set signal handlers */
   signal(SIGTERM, sighandler);
   signal(SIGINT, sighandler);
   /* Start all threads */
   pthread_mutex_lock(&d->startmutex);
   d->start = 1;
   pthread_mutex_unlock(&d->startmutex);
   pthread_cond_broadcast(&d->startcond);
   /* Wait for done signal */
   while(!done) {
      sleep(1);
      pthread_mutex_lock(&d->startmutex);
      if(active_threads == 0) done = 1;
      pthread_mutex_unlock(&d->startmutex);
   }
   /* Cancel all threads */
   for(bm_datastream_t s = d->streams;
       s != NULL;
       s = s->next)
      pthread_cancel(s->thread);
   /* Wait for all threads to be done */
   for(bm_datastream_t s = d->streams;
       s != NULL;
       s = s->next)
      pthread_join(s->thread, NULL);
}

/****************************************/
/****************************************/
