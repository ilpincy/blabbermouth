#include "bm_bt_datastream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

/****************************************/
/****************************************/

void bm_bt_datastream_destroy(void* ds);
int bm_bt_datastream_connect(void* ds);
void bm_bt_datastream_disconnect(void* ds);
ssize_t bm_bt_datastream_send(void* ds, const uint8_t* data, size_t sz);
ssize_t bm_bt_datastream_recv(void* ds, uint8_t* data, size_t sz);

/****************************************/
/****************************************/

void bm_bt_datastream_destroy(void* ds) {
}

/****************************************/
/****************************************/

int bm_bt_datastream_connect(void* ds) {
   return 0;
}

/****************************************/
/****************************************/

void bm_bt_datastream_disconnect(void* ds) {
}

/****************************************/
/****************************************/

ssize_t bm_bt_datastream_send(void* ds,
                              const uint8_t* data,
                              size_t sz) {
   return -1;
}

/****************************************/
/****************************************/

ssize_t bm_bt_datastream_recv(void* ds,
                              uint8_t* data,
                              size_t sz) {
   return -1;
}

/****************************************/
/****************************************/

bm_bt_datastream_t bm_bt_datastream_new(const char* desc) {
   /* Allocate memory */
   bm_bt_datastream_t this = malloc(sizeof(struct bm_bt_datastream_s));
   /* Initialize parent */
   bm_datastream_init(&this->parent,
                      desc,
                      bm_bt_datastream_destroy,
                      bm_bt_datastream_connect,
                      bm_bt_datastream_disconnect,
                      bm_bt_datastream_send,
                      bm_bt_datastream_recv);
   if(this->parent.status == BM_DATASTREAM_ERROR) {
      bm_bt_datastream_destroy(this);
      return NULL;
   }
   return this;
}

/****************************************/
/****************************************/

int bm_bt_scan() {
   /* Get the id of the first available Bluetooth device */
   int dev_id = hci_get_route(NULL);
   /* Open a socket to the device */
   int sock = hci_open_dev(dev_id);
   if (dev_id < 0 || sock < 0) {
      perror("opening socket");
      return 0;
   }
   /* Perform the Bluetooth scan
    * The scan lasts 8 * 1.28 seconds
    * The maximum number of devices reported is 255
    */
   int dev_max = 255;
   int scan_len = 8;
   fprintf(stdout, "Performing Bluetooth scan for %.2f seconds...\n", (1.28 * scan_len));
   inquiry_info* ii = (inquiry_info*)malloc(dev_max * sizeof(inquiry_info));
   int num_dev = hci_inquiry(dev_id, scan_len, dev_max, NULL, &ii, IREQ_CACHE_FLUSH);
   if(num_dev < 0) {
      perror("hci_inquiry");
      return 0;
   }
   /* Get the human-readable name of each device */
   char addr[19];
   char name[248];
   for(int i = 0; i < num_dev; ++i) {
      ba2str(&(ii+i)->bdaddr, addr);
      memset(name, 0, sizeof(name));
      if(hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), name, 0) < 0)
         strcpy(name, "[unknown]");
      printf("#%d: %s %s\n", (i+1), addr, name);
   }
   /* Cleanup */
   free(ii);
   close(sock);
   return 1;
}

/****************************************/
/****************************************/
