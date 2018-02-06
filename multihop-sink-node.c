#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

struct neighbor {
  struct neighbor *next;
  linkaddr_t addr;
  /* Counter for the divider */
  uint8_t divider;
  uint16_t last_rssi, last_lqi;
};
/* Defining the maximum amount of neighbors we can remember. */
#define MAX_NEIGHBORS 80

/* A memory pool from which we allocate neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list that holds the neighbors we have seen so far. */
LIST(neighbors_list1);
LIST(neighbors_list2);
LIST(neighbors_list3);
LIST(neighbors_list4);

/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast");
PROCESS(unicast_process, "Unicast");
AUTOSTART_PROCESSES(&broadcast_process, &unicast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
}
/*---------------------------------------------------------------------------*/
static void
unicast_recv(struct unicast_conn *c, const linkaddr_t *from) {
  struct neighbor *n;
  struct neighbor *s;
  
  printf("Sink: Table received from: %d.%d\n",
         from->u8[0], from->u8[1]);
         
  /* Assigning the neighbor to the sender node */
  n = packetbuf_dataptr();
    
  /* Allocating neighbor table based on sender adress */
  switch(n->addr.u8[0]) {
    
    case 1:
      for(n = n + 1; n->addr.u8[0] != 0; n = n + 1) {
        s = memb_alloc(&neighbors_memb);
        s = n;
        list_add(neighbors_list1, s);
      }
      printf("Table 1 filled!");
      for(n = list_head(neighbors_list1); n != NULL; n = list_item_next(n)){
        printf("Record from %d.%d RSSI %u, LQI %u\n", n->addr.u8[0], n->addr.u8[1], n->last_rssi, n->last_lqi);
      }
    break;
    /*---------------------------------------------------------------------------*/
    case 2:
      for(n = n + 1; n->addr.u8[0] != 0; n = n + 1) {
        s = memb_alloc(&neighbors_memb);
        s = n;
        list_add(neighbors_list2, s);
      }
      printf("Table 2 filled!");
      for(n = list_head(neighbors_list2); n != NULL; n = list_item_next(n)){
        printf("Record from %d.%d RSSI %u, LQI %u\n", n->addr.u8[0], n->addr.u8[1], n->last_rssi, n->last_lqi);
      }
    break;
    /*---------------------------------------------------------------------------*/
    case 3:
      for(n = n + 1; n->addr.u8[0] != 0; n = n + 1) {
        s = memb_alloc(&neighbors_memb);
        s = n;
        list_add(neighbors_list3, s);
      }
      printf("Table 3 filled!");
      for(n = list_head(neighbors_list3); n != NULL; n = list_item_next(n)){
        printf("Record from %d.%d RSSI %u, LQI %u\n", n->addr.u8[0], n->addr.u8[1], n->last_rssi, n->last_lqi);
      }
    break;
    /*---------------------------------------------------------------------------*/
    case 4:
      for(n = n + 1; n->addr.u8[0] != 0; n = n + 1) {
        s = memb_alloc(&neighbors_memb);
        s = n;
        list_add(neighbors_list4, s);
      }
      printf("Table 4 filled!");
      for(n = list_head(neighbors_list4); n != NULL; n = list_item_next(n)){
        printf("Record from %d.%d RSSI %u, LQI %u\n", n->addr.u8[0], n->addr.u8[1], n->last_rssi, n->last_lqi);
      }
    break;
  }
}
/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_call = {unicast_recv};
static struct unicast_conn unicast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data) {

  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 30 + random_rand() % (CLOCK_SECOND * 10));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Please, send!", 13);
    broadcast_send(&broadcast);
    printf("broadcast message sent\n");
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data) {

  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_call);
    
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
