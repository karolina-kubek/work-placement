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
  uint8_t last_hello;
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
LIST(neighbors_list5);
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
  
  //Choosing a list to save to , based on the source address
  switch(n->addr.u8[0]) {
    case 1:
      s = list_head(neighbors_list1);
    break;
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
  }
  
  // Writing all the received data into the lists
  for(n = n + 1; n->addr.u8[0] != 0; n = n + 1) {
    s->addr.u8[0] = n->addr.u8[0];
    s->addr.u8[1] = n->addr.u8[1];
    s->divider = n->divider;
    s->last_rssi = n->last_rssi;
    s->last_lqi = n->last_lqi;
    s->last_hello = n->last_hello;
    printf("Neighbour Address: %d.%d Divider: %u RSSI: %u LQI: %u Hello(time second): %u\n", s->addr.u8[0], s->addr.u8[1], s->divider, s->last_rssi, s->last_lqi, s->last_hello);
    s = list_item_next(s);
  }
  while(s->addr.u8[0] != 0) {
    s->addr.u8[0] = 0;
    s->addr.u8[1] = 0;
    s->divider = 0;
    s->last_rssi = 0;
    s->last_lqi = 0;
    s->last_hello = 0;
    
    s = list_item_next(s);
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
static const struct unicast_callbacks unicast_call = {unicast_recv};
static struct unicast_conn unicast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data) {

  static struct etimer et;
  struct neighbor *s;
  
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();
  int i;
   /*Writing to the memory*/
  for(i = 0; i < 5; i++) {
    s = memb_alloc(&neighbors_memb);
    list_add(neighbors_list1, s);
    s = memb_alloc(&neighbors_memb);
    list_add(neighbors_list2, s);
    s = memb_alloc(&neighbors_memb);
    list_add(neighbors_list3, s);
    s = memb_alloc(&neighbors_memb);
    list_add(neighbors_list4, s);
    s = memb_alloc(&neighbors_memb);
    list_add(neighbors_list5, s);
  }
  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 60 - 62seconds */
    etimer_set(&et, CLOCK_SECOND * 60 + random_rand() % (CLOCK_SECOND * 2));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Please, send!", 13);
    broadcast_send(&broadcast);
    printf("broadcast message sent to ask for a table\n");
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
