#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/leds.h"
#include <stdio.h>

/* This structure holds information about neighbors. */
struct neighbor {
  struct neighbor *next;
  linkaddr_t addr;
  /* Counter for the divider */
  uint8_t divider;
  uint16_t last_rssi, last_lqi;
  /* A number thet keeps on check how many messages were recieved out of 12  */
  uint8_t reliability_counter;
};
/* Dfining the maximum amount of neighbors we can remember. */
#define MAX_NEIGHBORS 20

/* A memory pool from which we allocate neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list that holds the neighbors we have seen so far. */
LIST(neighbors_list);
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  struct neighbor *n;
  linkaddr_t *addr;
  
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
         
    /* Check if we already know this neighbor. */
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
    /* We break out of the loop if the address of the neighbor matches
       the address of the neighbor from which we received this
       broadcast message. */
    if(linkaddr_cmp(&n->addr, from)) {  
      /* Compute average */
      n->last_rssi = (uint16_t)((n->last_rssi * n->divider) + packetbuf_attr(PACKETBUF_ATTR_RSSI) / (n->divider + 1));
      n->last_lqi = (uint16_t)((n->last_lqi * n->divider) + packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY) / (n->divider + 1));
      n->divider = n->divider + 1;
      //n->last_lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
      n->reliability_counter = n->reliability_counter + 1;
      break;
    }
  }
  
  /* If n is NULL, this neighbor was not found in our list, and we
   allocate a new struct neighbor from the neighbors_memb memory
   pool. */
  if(n == NULL) {
    n = memb_alloc(&neighbors_memb);

    /* If we could not allocate a new neighbor entry, we give up. We
       could have reused an old neighbor entry, but we do not do this
       for now. */
    if(n == NULL) {
      return;
    }

    /* Initialize the address field. */
    linkaddr_copy(&n->addr, from);
    linkaddr_copy(addr, from);
    /* We can now fill in the fields in our neighbor entry. */
    n->last_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    n->last_lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
    n->divider = 1;
     
    /* Place the neighbor on the neighbor list. */
    list_add(neighbors_list, n);
  }
  
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
    
    linkaddr_copy(addr, &n->addr);
    printf("Record from %d.%d RSSI %u, LQI %u\n", addr->u8[0], addr->u8[1], n->last_rssi, n->last_lqi);
  }
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Hello", 6);
    broadcast_send(&broadcast);
    printf("broadcast message sent\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
