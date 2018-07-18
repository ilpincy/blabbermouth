#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "bm_udp_datastream.h"
#include "bm_debug.h"

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
   /* Get protocol (and discard it) */
   tok = strtok_r(NULL, ":", &saveptr);
   /* Get verbosity (and discard it) */
   tok = strtok_r(NULL, ":", &saveptr);
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
      /* Let's try this interface... */
      this->stream = socket(iface->ai_family,
                            iface->ai_socktype,
                            iface->ai_protocol);
      if(this->stream > 0) {
         /* We have a socket, let's save it */
         memcpy(&this->sock, iface->ai_addr, sizeof(this->sock));
         freeaddrinfo(ifaceinfo);
         bm_datastream_set_status(ds, BM_DATASTREAM_READY, "ready");
         /* Send a HELLO message to start up the connection */
         uint8_t hello = 0;
         bm_udp_datastream_send(this, &hello, 1);
         return 1;
      }
   }
   /* None of the interfaces worked, error */
   bm_datastream_set_status(this,
                            BM_DATASTREAM_ERROR,
                            strerror(errno));
   return 0;
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
   }
   bm_datastream_set_status(ds, BM_DATASTREAM_UNKNOWN, "unknown");
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
      bm_debug(ds, "send: sending %zd bytes", tot);
      sent = sendto(this->stream, data, tot, 0, (struct sockaddr*)(&this->sock), sizeof(this->sock));
      bm_debug(ds, "send: sent %zd bytes", sent);
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
   socklen_t addrlen;
   while(tot > 0) {
      bm_debug(ds, "recv: waiting for %zd bytes", tot);
      received = recvfrom(this->stream, data, tot, 0, (struct sockaddr*)(&this->sock), &addrlen);
      bm_debug(ds, "recv: received %zd bytes", received);
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
   memset(&this->sock, 0, sizeof(this->sock));
   /* All done */
   return this;
}

/****************************************/
/****************************************/
