#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "bm_datastream.h"

/****************************************/
/****************************************/

void bm_datastream_init(bm_datastream_t ds,
                        const char* desc,
                        void (*destroyf)(void*),
                        int (*connectf)(void*),
                        void (*disconnectf)(void*),
                        ssize_t (*sendf)(void*, const uint8_t*, size_t),
                        ssize_t (*recvf)(void*, uint8_t*, size_t)) {
   /* Set methods */
   ds->destroy = destroyf;
   ds->connect = connectf;
   ds->disconnect = disconnectf;
   ds->send = sendf;
   ds->recv = recvf;
   /* Set descriptor */
   ds->descriptor = strdup(desc);
   /* Set id */
   char* delim = strchr(desc, ':');
   if(!delim) {
      bm_datastream_set_status(ds,
                               BM_DATASTREAM_ERROR,
                               "Can't parse '%s'",
                               desc);
      return;
   }
   ds->id = (char*)malloc(delim - desc + 1);
   strncpy(ds->id, desc, delim - desc);
   ds->id[delim - desc + 1] = '\0';
   /* Set status */
   bm_datastream_set_status(ds, BM_DATASTREAM_UNKNOWN, "unknown");
   /* Set next */
   ds->next = NULL;
}

/****************************************/
/****************************************/

void bm_datastream_destroy(bm_datastream_t ds) {
   ds->disconnect(ds);
   free(ds->status_desc);
   free(ds->descriptor);
   free(ds->id);
}

/****************************************/
/****************************************/

void bm_datastream_set_status(void* ds,
                              int status,
                              const char* desc,
                              ...) {
   bm_datastream_t this = (bm_datastream_t)ds;
   this->status = status;
   free(this->status_desc);
   va_list al;
   va_start(al, desc);
   vasprintf(&this->status_desc, desc, al);
   va_end(al);
}

/****************************************/
/****************************************/
