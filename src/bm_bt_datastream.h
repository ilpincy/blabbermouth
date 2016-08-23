#ifndef BM_BT_DATASTREAM_H
#define BM_BT_DATASTREAM_H

#include "bm_datastream.h"

struct bm_bt_datastream_s {
   /* Generic datastream definition */
   struct bm_datastream_s parent;
   /* Socket stream */
   int stream;
};
typedef struct bm_bt_datastream_s* bm_bt_datastream_t;

/*
 * Creates a new BT datastream.
 * @return The new BT datastream.
 */
extern bm_bt_datastream_t bm_bt_datastream_new();

/*
 * Performs a scan of the Bluetooth devices around.
 * Prints the results on the screen, or an error message.
 * @return 1 if all is OK, 0 in case of error.
 */
extern int bm_bt_scan();

#endif
