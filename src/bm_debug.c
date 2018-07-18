#define _GNU_SOURCE
#include "bm_debug.h"
#include "bm_datastream.h"
#include <stdio.h>
#include <stdarg.h>

/****************************************/
/****************************************/

void bm_debug(void* ds,
              const char* fmt, ...) {
   bm_datastream_t this = (bm_datastream_t)ds;
   if(!this->verbose) return;
   char* msg;
   va_list al;
   va_start(al, fmt);
   vasprintf(&msg, fmt, al);
   va_end(al);
   fprintf(stderr, "[%s] %s\n", this->descriptor, msg);
}

/****************************************/
/****************************************/
