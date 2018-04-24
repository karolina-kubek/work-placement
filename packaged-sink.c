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
};


#define MAX_NEIGHBORS 16
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);
LIST(neighbors_list);

static uint8_t flag = 0;
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast");
PROCESS(total_PDR_timer, "Total PDR timer");
PROCESS(retransmission_PDR_timer, "Retransmission PDR timer");
AUTOSTART_PROCESSES(&example_broadcast_process, &total_PDR_timer, &retransmission_PDR_timer);
/*---------------------------------------------------------------------------*/

#define FILENAME "test"
/*---------------------------------------------------------------------------*/
  /* Formatting is needed if the storage device is in an unknown state;
   e.g., when using Coffee on the storage device for the first time. */
#ifndef NEED_FORMATTING
#define NEED_FORMATTING 0
#endif

/*---------------------------------------------------------------------------*/
static int
file_test(const char *filename)
{
  static struct neighbor *n;
  int fd;                  /* File discriptor */
  int r;                   /* Record written into the open file */
  
  struct record {
    linkaddr_t addr;
    uint8_t id;
    uint8_t PDR_avg;
    uint8_t divider;
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
     reads and writes. */
     
  fd = cfs_open(FILENAME, CFS_WRITE | CFS_APPEND | CFS_READ);
  if(fd < 0) {
    printf("failed to open %s\n", FILENAME);
    return 0;
  }
  
/*---------------------------------------------------------------------------*/
  /* Write the struct into the open file */
 
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
     beginning of the file. */
     
  if(cfs_seek(fd, 0, CFS_SEEK_SET) != 0) {
    printf("seek failed\n");
    cfs_close(fd);
    return 0;
  }
  
/*---------------------------------------------------------------------------*/
  /* Read - Output */
  /* Read all written records and print their contents. */
  
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
  /* Referance -> cfs_open */
  
  cfs_close(fd);

  return 1;
  
}

/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  /*uint8_t neighbour_id;     
  struct neighbor *n;  
  printf("broadcast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);

  neighbour_id = 1;	 
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {

     We break out of the loop if the address of the neighbor matches
       the address of the neighbor from which we received this
       broadcast message. 
    if(linkaddr_cmp(&n->addr, from)) {
      break;
    }
    neighbour_id++;
  }

   If n is NULL, this neighbor was not found in our list, and we
     allocate a new struct neighbor from the neighbors_memb memory
     pool. 
  if(n == NULL) {
    n = memb_alloc(&neighbors_memb);

    /* If we could not allocate a new neighbor entry, we give up. We
       could have reused an old neighbor entry, but we do not do this
       for now. 
    if(n == NULL) {
      return;
    }

    /Initialize the fields. 
    linkaddr_copy(&n->addr, from);
    n->id = neighbour_id;
    printf("Neighbour id: %u\n", n->id);
    n->sno1 = 0;
    n->sno2 = 0;
    n->sno3 = 0;
    n->PDR_avg = 0;
    n->divider = 0;

     Place the neighbor on the neighbor list. 
    list_add(neighbors_list, n);
  } else {
    if( n->sno1 == 0 ) {
      n->sno1 = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    } else if( n->sno2 == 0 ) {
      n->sno2 = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    } else if( n->sno3 == 0 ) {
      n->sno3 = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    } else {
      printf("Number out of range!\n");
    }
  }
  flag = neighbour_id;
  */
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(total_PDR_timer, ev, data)
{

  struct neighbor *n;
  static struct etimer et; 
  
  PROCESS_BEGIN();
  
  while(1) {
    /* Delay process for an hour and a bit(longest node synch time) */
    etimer_set(&et, CLOCK_SECOND * 3800);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    file_test(FILENAME);
    
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(retransmission_PDR_timer, ev, data)
{

  static struct neighbor *n;
  static struct etimer et, xt; 
  static uint8_t avg = 0;
  PROCESS_BEGIN();
  
  while(1) {
    etimer_set(&et, CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
      
      if(n->id == flag) {
        etimer_set(&xt, CLOCK_SECOND * 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&xt));
        printf("First sqno: %u, second: %u, third: %u\n", n->sno1, n->sno2, n->sno3);
        
        if( ( (((n->sno1)+1) == n->sno2) && (((n->sno2)+1) == n->sno3) && (((n->sno1)+2) == n->sno3) ) || (n->sno1 == 255 && n->sno2 == 1 && n->sno3 == 0) ) {
          avg = 100;
          printf("Retransmission avg: 100%\n");
        } else if( (((n->sno1)+1) == n->sno2) || (((n->sno2)+1) == n->sno3) || (((n->sno1)+2) == n->sno3) ){
          avg = 66;
          printf("Retransmission avg: 66%\n");
        } else {
          avg = 33;
          printf("Retransmission avg: 33%\n");
        }
        if( n->PDR_avg == 0 ) {
          n->PDR_avg = avg;
        } else {
          n->PDR_avg = (uint8_t)(( (n->PDR_avg * n->divider) + avg ) / (n->divider+1));
        }
        n->divider = n->divider + 1;
        printf("Retransmission PDR: %u Divider: %u Node: %u.%u\n", n->PDR_avg, n->divider, n->addr.u8[0], n->addr.u8[1]);
        flag = 0;
        n->sno1 = 0;
        n->sno2 = 0;
        n->sno3 = 0;
        
        break;
        
      }
    }
    
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;
  static uint8_t time_stamp;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);
  
  while(1) {
    /* Delay 5 minutes */
    etimer_set(&et, CLOCK_SECOND * 300);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    time_stamp = clock_seconds();
    printf("Clock time: %u\n", time_stamp);
    packetbuf_copyfrom(&time_stamp, sizeof(uint8_t));
    broadcast_send(&broadcast);
    printf("broadcast message sent\n");
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
