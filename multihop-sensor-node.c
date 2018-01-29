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
static uint8_t sink_addr_0 = 0;
static uint8_t sink_addr_1 = 0;
static uint8_t flag = 1;
/* Dfining the maximum amount of neighbors we can remember. */
#define MAX_NEIGHBORS 20

/* A memory pool from which we allocate neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list that holds the neighbors we have seen so far. */
LIST(neighbors_list);
/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast");
PROCESS(unicast_process, "Unicast");
AUTOSTART_PROCESSES(&broadcast_process, &unicast_process);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{

  if( strcmp((char *)packetbuf_dataptr(), "Hello") == 0) {
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
  } else if (strcmp((char *)packetbuf_dataptr(), "Please, send!") == 0) {
       
    if (sink_addr_0 == 0) {  
      //static struct etimer et;
        
      printf("broadcast message received from sink : %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
  
      sink_addr_0 = from->u8[0];
      sink_addr_1 = from->u8[1];
      flag = 0; 
      /* Delay 2-4 seconds and rebroadcast*/
      //etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));
      //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));  
      packetbuf_copyfrom((char *)packetbuf_dataptr(), 14);
      broadcast_send(c);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
unicast_recv(struct unicast_conn *c, const linkaddr_t *from)
{
  static linkaddr_t addr;
  
  printf("Unicast message received from: %d.%d Sending to the sink\n",
         from->u8[0], from->u8[1]);
  packetbuf_copyfrom(packetbuf_dataptr(), sizeof(packetbuf_dataptr()));
  addr.u8[0] = sink_addr_0;
  addr.u8[1] = sink_addr_1; 
  unicast_send(c, &addr);

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
    etimer_set(&et, CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Hello", 6);
    broadcast_send(&broadcast);
    printf("broadcast message sent\n");
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data) {

  static struct etimer et;
  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_call);
  
  while(1){
    linkaddr_t addr;
      /*Etimer set to dely by 5 sec*/
    etimer_set(&et, CLOCK_SECOND * 5);
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    if( sink_addr_0 != 0 ) {
      addr.u8[0] = sink_addr_0;
      addr.u8[1] = sink_addr_1;
    
      if(!linkaddr_cmp(&addr, &linkaddr_node_addr) && flag == 0) {
    
        packetbuf_copyfrom(neighbors_list, sizeof(neighbors_list));
        unicast_send(&unicast, &addr);
        flag = 1;
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/