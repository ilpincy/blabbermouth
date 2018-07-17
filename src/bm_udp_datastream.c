#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "bm_udp_datastream.h"

/****************************************/
/****************************************/

void bm_udp_datastream_destroy(void* ds);
int bm_udp_datastream_connect(void* ds);
void bm_udp_datastream_disconnect(void* ds);
ssize_t bm_udp_datastream_send(void* ds, const uint8_t* data, size_t sz);
ssize_t bm_udp_datastream_recv(void* ds, uint8_t* data, size_t sz);

/****************************************/
/****************************************/

int bm_udp_datastream_parse(bm_udp_datastream_t ds,
                            const char* desc) {
   /* Duplicate string for strtok_r */
   char* wdesc = strdup(desc);
   /* Buffer pointer for strtok_r */
   char* saveptr = NULL;
   /* Get id (and discard it) */
   char* tok = strtok_r(wdesc, ":", &saveptr);
   if(!tok) {
      bm_datastream_set_status(ds,
                               BM_DATASTREAM_ERROR,
                               "Can't parse '%s'",
                               desc);
      free(wdesc);
      return 0;
   }
   /* Get protocol */
   tok = strtok_r(NULL, ":", &saveptr);
   if(!tok) {
      bm_datastream_set_status(ds,
                               BM_DATASTREAM_ERROR,
                               "Can't parse '%s'",
                               desc);
      free(wdesc);
      return 0;
   }
   if(strcmp(tok, "udp") != 0) {
      bm_datastream_set_status(ds,
                               BM_DATASTREAM_ERROR,
                               "Stream descriptor '%s' is not a udp stream",
                               desc);
      free(wdesc);
      return 0;
   }
   /* Get server */
   tok = strtok_r(NULL, ":", &saveptr);
   if(!tok) {
      bm_datastream_set_status(ds,
                               BM_DATASTREAM_ERROR,
                               "Can't parse server in '%s'",
                               desc);
      free(wdesc);
      return 0;
   }
   ds->server = strdup(tok);
   /* Get port */
   tok = strtok_r(NULL, ":", &saveptr);
   if(!tok) {
      bm_datastream_set_status(ds,
                               BM_DATASTREAM_ERROR,
                               "Can't parse port in '%s'",
                               desc);
      free(wdesc);
      return 0;
   }
   ds->port = strdup(tok);
   /* Cleanup */
   free(wdesc);
   /* All is OK */
   return 1;
}

/****************************************/
/****************************************/

void bm_udp_datastream_destroy(void* ds) {
   bm_udp_datastream_t this = (bm_udp_datastream_t)ds;
   bm_datastream_destroy(&this->parent);
   free(this);
}

/****************************************/
/****************************************/

int bm_udp_datastream_connect(void* ds) {
   /* Cast datastream to this type */
   bm_udp_datastream_t this = (bm_udp_datastream_t)ds;
   /* Disconnect if the stream is already connected */
   if(this->stream != -1)
      bm_udp_datastream_disconnect(this);
   /* Used to store the return value of the network function calls */
   int retval;
   /* Get information on the available interfaces */
   struct addrinfo hints, *ifaceinfo;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;      /* Only IPv4 is accepted */
   hints.ai_socktype = SOCK_DGRAM; /* UDP socket */
   retval = getaddrinfo(this->server,
                        this->port,
                        &hints,
                        &ifaceinfo);
   if(retval != 0) {
      bm_datastream_set_status(this,
                               BM_DATASTREAM_ERROR,
                               "%s: Error getting address information: %s",
                               this->parent.descriptor,
                               gai_strerror(retval));
      return 0;
   }
   /* Bind on the first interface available */
   this->stream = -1;
   struct addrinfo* iface = NULL;
   for(iface = ifaceinfo;
       (iface != NULL) && (this->stream == -1);
       iface = iface->ai_next) {
      this->stream = socket(iface->ai_family,
                            iface->ai_socktype,
                            iface->ai_protocol);
      if(this->stream > 0) {
         if(connect(this->stream,
                    iface->ai_addr,
                    iface->ai_addrlen) == -1) {
            this->stream = -1;
            bm_datastream_set_status(this,
                                     BM_DATASTREAM_ERROR,
                                     strerror(errno));
            return 0;
         }
      }
   }
   freeaddrinfo(ifaceinfo);
   bm_datastream_set_status(ds, BM_DATASTREAM_READY, "ready");
   return 1;
}

/****************************************/
/****************************************/

void bm_udp_datastream_disconnect(void* ds) {
   /* Cast datastream to this type */
   bm_udp_datastream_t this = (bm_udp_datastream_t)ds;
   if(this->stream != -1) {
      /* Close stream */
      close(this->stream);
      this->stream = -1;
      bm_datastream_set_status(ds, BM_DATASTREAM_UNKNOWN, "unknown");
   }
}

/****************************************/
/****************************************/

ssize_t bm_udp_datastream_send(void* ds,
                               const uint8_t* data,
                               size_t sz) {
   /* Cast datastream to this type */
   bm_udp_datastream_t this = (bm_udp_datastream_t)ds;
   /* Make sure stream is ready */
   if(this->parent.status != BM_DATASTREAM_READY) return -1;
   /* To keep track of how many bytes have been sent */
   ssize_t tot = sz, sent;
   /* Keep sending until done or error */
   while(tot > 0) {
      sent = send(this->stream, data, tot, 0);
      if(sent < 0) {
         bm_udp_datastream_disconnect(this);
         bm_datastream_set_status(this,
                                  BM_DATASTREAM_ERROR,
                                  "Error sending data: %s",
                                  strerror(errno));
         return sent;
      }
      tot -= sent;
      data += sent;
   }
   return sz;
}

/****************************************/
/****************************************/

ssize_t bm_udp_datastream_recv(void* ds,
                               uint8_t* data,
                               size_t sz) {
   /* Cast datastream to this type */
   bm_udp_datastream_t this = (bm_udp_datastream_t)ds;
   /* Make sure stream is ready */
   if(this->parent.status != BM_DATASTREAM_READY) return -1;
   /* To keep track of how many bytes have been received */
   ssize_t tot = sz, received;
   while(tot > 0) {
      received = recv(this->stream, data, tot, 0);
      if(received < 0){
         bm_udp_datastream_disconnect(this);
         bm_datastream_set_status(this,
                                  BM_DATASTREAM_ERROR,
                                  "Error receiving data: %s",
                                  strerror(errno));
         return received;
      }
      if(received == 0) return 0;
      tot -= received;
      data += received;
   }
   return sz;
}

/****************************************/
/****************************************/

bm_udp_datastream_t bm_udp_datastream_new(const char* desc) {
   /* Allocate memory */
   bm_udp_datastream_t this = malloc(sizeof(struct bm_udp_datastream_s));
   /* Initialize parent */
   bm_datastream_init(&this->parent,
                      desc,
                      bm_udp_datastream_destroy,
                      bm_udp_datastream_connect,
                      bm_udp_datastream_disconnect,
                      bm_udp_datastream_send,
                      bm_udp_datastream_recv);
   if(this->parent.status == BM_DATASTREAM_ERROR) {
      bm_udp_datastream_destroy(this);
      return NULL;
   }
   /* Set local attributes */
   this->stream = -1;
   if(!bm_udp_datastream_parse(this, desc)) {
      bm_udp_datastream_destroy(this);
      return NULL;
   }
   /* All done */
   return this;
}

/****************************************/
/****************************************/
