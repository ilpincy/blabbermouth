#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <config.h>
#include "bm_dispatcher.h"
#include "bm_bt_datastream.h"

/****************************************/
/****************************************/

void usage(FILE* stream, const char* prg) {
   fprintf(stream, "Usage:\n");
   fprintf(stream, "   %s <-s SIZE> [-f FILE]... [STREAM]...\n", prg);
   fprintf(stream, "   %s scan\n", prg);
   fprintf(stream, "Data repeater on various types of connections.\n");
   fprintf(stream, "\nBlabbermouth has two operational modes: streaming and scanning.\n");
   fprintf(stream, "\n== STREAMING ==\n\n");
   fprintf(stream, "In streaming mode, BlabberMouth connects to each STREAM passed as command line\n");
   fprintf(stream, "parameter and/or in FILE. Every time a message is sent by one of the peers over\n");
   fprintf(stream, "a stream, BlabberMouth collects the data and sends it over the other streams.\n\n");
   fprintf(stream, "Each message managed by BlabberMouth must be exactly SIZE bytes long, so the\n");
   fprintf(stream, "-s option is required.\n\n");
   fprintf(stream, "The syntax for stream descriptors is: ID:TYPE:DATA, where ID is a unique\n");
   fprintf(stream, "identifier for the stream; TYPE is a case-sensitive string such as 'tcp', 'udp',\n");
   fprintf(stream, "'bt', or 'xbee'; and DATA is a colon-separated string of fields that specify\n");
   fprintf(stream, "how to connect to the stream.\n\n");
   fprintf(stream, "Supported stream descriptors:\n\n");
   fprintf(stream, "  ID:tcp:SERVER:PORT      A TCP connection to SERVER on PORT\n");
   fprintf(stream, "  ID:udp:SERVER:PORT      A UDP connection to SERVER on PORT\n");
#ifdef BLABBERMOUTH_WITH_BT
   fprintf(stream, "  ID:bt:rfcomm:CHANNEL    An RFComm Bluetooth connection on CHANNEL\n");
#endif
   /* fprintf(stream, "  ID:xbee:ADDRESS:PORT    An XBee connection to ADDRESS on PORT\n"); */
   fprintf(stream, "\nOptions:\n\n");
   fprintf(stream, "  -s SIZE | --size SIZE   The size (in bytes) of a message\n");
   fprintf(stream, "  -f FILE | --file FILE   A file containing one stream descriptor per line\n");
   fprintf(stream, "\n== SCANNING ==\n\n");
   fprintf(stream, "In scanning mode, Blabbermouth looks for Bluetooth devices to connect to and\n");
   fprintf(stream, "prints a list of available devices. BlueZ must be installed for Bluetooth to be\n");
   fprintf(stream, "supported.\n");
   fprintf(stream, "\n");
}

/****************************************/
/****************************************/

int file_parse(const char* fn,
               bm_dispatcher_t d) {
   /* Open the file */
   FILE* fd= fopen(fn, "r");
   if(!fd) {
      fprintf(stderr,
              "Can't open file '%s': %s\n",
              fn,
              strerror(errno));
      return 0;
   }
   /* Go through its content */
   char* line = NULL;
   char* start;
   char* end;
   size_t linelen = 0;
   while(getline(&line, &linelen, fd) >= 0) {
      /* Trim leading whitespace */
      start = line;
      while(*start != '\0' && isspace(*start)) ++start;
      /* Make sure the line is not empty or a comment */
      if(*start != '\0' && *start != '#') {
         /* Trim trailing whitespace */
         end = start + strlen(start) - 1;
         while(end != start && isspace(*end)) --end;
         *(end+1) = '\0';
         if(!bm_dispatcher_stream_add(d, start)) {
            free(line);
            fclose(fd);
            return 0;
         }
      }
   }
   /* All done */
   free(line);
   fclose(fd);
   return 1;
}

/****************************************/
/****************************************/

int main(int argc, char* argv[]) {
   /* Check whether arguments have been given */
   if(argc < 2) {
      usage(stdout, argv[0]);
      return EXIT_SUCCESS;
   }
   /* Check the first argument to detect the mode */
   if(strcmp(argv[1], "scan") == 0) {
      /* Scanning mode */
      if(argc > 2) {
         fprintf(stderr, "%s: mode 'scan' accepts no options\n", argv[0]);
         return EXIT_FAILURE;
      }
#ifdef BLABBERMOUTH_WITH_BT
      /* Execute bluetooth scan */
      if(!bm_bt_scan())
         return EXIT_FAILURE;
#endif
   }
   else {
      /* Streaming mode */
      /* Create the stream dispatcher */
      bm_dispatcher_t d = bm_dispatcher_new();
      /* Parse the arguments */
      for(int i = 1; i < argc; ++i) {
         /* Check options */
         if(argv[i][0] == '-') {
            /* Check whether argument starts with -f or --file */
            if(strcmp(argv[i], "-f") == 0 ||
               strcmp(argv[i], "--file") == 0) {
               ++i;
               if(i >= argc) {
                  fprintf(stderr, "%s: expected file after -f and --file\n", argv[0]);
                  return EXIT_FAILURE;
               }
               fprintf(stdout, "Reading streams from %s\n", argv[i]);
               if(!file_parse(argv[i], d)) {
                  /* Some error occurred */
                  bm_dispatcher_destroy(d);
                  return EXIT_FAILURE;
               }
            }
            else if(strcmp(argv[i], "-s") == 0 ||
                    strcmp(argv[i], "--size") == 0) {
               ++i;
               if(i >= argc) {
                  fprintf(stderr, "%s: expected size value after -s and --size\n", argv[0]);
                  bm_dispatcher_destroy(d);
                  return EXIT_FAILURE;
               }
               char* endptr;
               d->msg_len = strtol(argv[i], &endptr, 10);
               if(endptr == argv[i] || *endptr != '\0') {
                  fprintf(stderr, "%s: can't parse '%s' as a number\n", argv[0], argv[i]);
                  bm_dispatcher_destroy(d);
                  return EXIT_FAILURE;
               }
            }
            else {
               fprintf(stderr, "%s: %s: unknown option\n", argv[0], argv[i]);
               bm_dispatcher_destroy(d);
               return EXIT_FAILURE;
            }
         }
         else {
            /* Not an option, consider it a stream descriptor */
            bm_dispatcher_stream_add(d, argv[i]);
         }
      }
      /* Make sure required information has been passed */
      if(d->msg_len == 0) {
         fprintf(stderr, "%s: option -s SIZE is required, with a value for SIZE > 0\n", argv[0]);
         return EXIT_FAILURE;
      }
      /* Parsing done, start the execution */
      bm_dispatcher_execute(d);
      /* All done */
      bm_dispatcher_destroy(d);
   }
   return EXIT_SUCCESS;
}

/****************************************/
/****************************************/
