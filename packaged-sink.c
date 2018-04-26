#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>
#include <string.h>
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

struct neighbor {
  struct neighbor *next;
  linkaddr_t addr;
  uint8_t id;
  uint8_t sno1;
  uint8_t sno2;
  uint8_t sno3;
  uint8_t PDR_avg;
  uint8_t divider;
  uint16_t last_rssi, last_lqi;
};


/* Defining the maximum amount of neighbors we can remember. */
#define MAX_NEIGHBORS 80

/* A memory pool from which we allocate neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list that holds the neighbors we have seen so far. */
LIST(neighbors_list6); 
LIST(neighbors_list2);
LIST(neighbors_list3);
LIST(neighbors_list4);
LIST(neighbors_list5);

static uint8_t flag = 0;
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/

#define FILENAME "test"
/*---------------------------------------------------------------------------*/
  /* Formatting is needed if the storage device is in an unknown state;
   e.g., when using Coffee on the storage device for the first time. */
#ifndef NEED_FORMATTING
#define NEED_FORMATTING 0
#endif

/*---------------------------------------------------------------------------
static int
file_test(const char *filename)
{
  static struct neighbor *n;
  int fd;                  /* File discriptor 
  int r;                   /* Record written into the open file 
  
  struct record {
  uint8_t id;
  uint8_t PDR_avg;
  uint8_t divider;
  uint16_t last_rssi, last_lqi;
  } record;
   
   
  cfs_remove(FILENAME);
  fd = cfs_open(FILENAME, CFS_READ);
  if(fd == -1) {
    printf("Successfully removed file\n");
  } else {
    printf("ERROR: could read from memory in step 6.\n");
  } 
     
  printf("SAVING TO FLASH!\n");
/*---------------------------------------------------------------------------*/
  /* Obtain a file descriptor for the file, capable of handling both
     reads and writes. 
     
  fd = cfs_open(FILENAME, CFS_WRITE | CFS_APPEND | CFS_READ);
  if(fd < 0) {
    printf("failed to open %s\n", FILENAME);
    return 0;
  }
  
/*---------------------------------------------------------------------------*/
  /* Write the struct into the open file 
 
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
    record.addr = n->addr;
    record.id = n->id;
    record.PDR_avg = n->PDR_avg;
    record.divider = n->divider;
    
    r = cfs_write(fd, &record, sizeof(record));
    if(r != sizeof(record)) {
      printf("failed to write %d bytes to %s\n",
           (int)sizeof(record), FILENAME);
      cfs_close(fd);
      return 0;
    }
  }
         
/*---------------------------------------------------------------------------*/
  /* Moving the pointer to the beginning */
  /* To read back the message, we need to move the file pointer to the
     beginning of the file. 
     
  if(cfs_seek(fd, 0, CFS_SEEK_SET) != 0) {
    printf("seek failed\n");
    cfs_close(fd);
    return 0;
  }
  
/*---------------------------------------------------------------------------*/
  /* Read - Output */
  /* Read all written records and print their contents. 
  
  for(;;) {
    r = cfs_read(fd, &record, sizeof(record));
    if(r == 0) {
      break;
    } else if(r < sizeof(record)) {
      printf("failed to read %d bytes from %s, got %d\n",
             (int)sizeof(record), FILENAME, r);
      cfs_close(fd);
      return 0;
    }

    printf("RECORD: id: \"%u\", avg: \"%u\", divider: %u\n",
           record.id, record.PDR_avg, record.divider);
    
  }
  
/*---------------------------------------------------------------------------*/
  /* Release the internal resources */
  /* Referance -> cfs_open 
  
  cfs_close(fd);

  return 1;
  
}

/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  struct neighbor *n;
  struct neighbor *s;

  if (*(unsigned int *) packetbuf_dataptr() != 0) {
  
    printf("Neighbour table received from %d.%d\n",
         from->u8[0], from->u8[1]);
    /* Assigning the neighbor to the sender node */
    n = packetbuf_dataptr();
  
    //Choosing a list to save to , based on the source address
    switch(n->addr.u8[0]) {
      case 2:
        s = list_head(neighbors_list2);
      break;
      case 3:
        s = list_head(neighbors_list3);
      break;
      case 4:
        s = list_head(neighbors_list4);
      break;
      case 5:
        s = list_head(neighbors_list5);
      break;
      case 6:
        s = list_head(neighbors_list6);
      break;
    }
  
    // Writing all the received data into the lists
    for(n = n + 1; n->addr.u8[0] != 0; n = n + 1) {
      s->addr.u8[0] = n->addr.u8[0];
      s->addr.u8[1] = n->addr.u8[1];
      
      s->id = n->id;
      s->sno1 = n->sno1;
      s->sno2 = n->sno2;
      s->sno3 = n->sno3;
      s->divider = n->divider;
      s->PDR_avg = n->PDR_avg;
      s->last_rssi = n->last_rssi;
      s->last_lqi = n->last_lqi;
      printf("Neighbour Address: %d.%d Divider: %u RSSI: %u LQI: %u\n", s->addr.u8[0], s->addr.u8[1], s->divider, s->last_rssi, s->last_lqi);
      s = list_item_next(s);
    }
    // Why am I doing this? Besacuse there are manky leftovers from a previous go.
    while(s->addr.u8[0] != 0) {
      s->addr.u8[0] = 0;
      s->addr.u8[1] = 0;
      s->divider = 0;
      s->PDR_avg = 0;
      s->last_rssi = 0;
      s->last_lqi = 0;
    
      s = list_item_next(s);
    }
  }
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
  static uint8_t time_stamp;
  static uint8_t i;
  static uint8_t table_request = 0;
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  
  while(1) {
    i = 0;
    while( i < 5 ) {
      /* Delay 5 minutes */
      etimer_set(&et, CLOCK_SECOND * 180);

      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
      time_stamp = clock_seconds();
      printf("Clock time: %u\n", time_stamp);
      packetbuf_copyfrom(&time_stamp, sizeof(uint8_t));
      broadcast_send(&broadcast);
      printf("Introduction request sent\n");
      i++;
    }
    
    etimer_set(&et, CLOCK_SECOND * 180);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    packetbuf_copyfrom(table_request, sizeof(uint8_t));
    broadcast_send(&broadcast);
    printf("Table forwarding request sent\n");
    
  }

  PROCESS_END();
}

