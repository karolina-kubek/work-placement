#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>

struct neighbor {
  struct neighbor *next;
  linkaddr_t addr;
  uint8_t received;
};

#define MAX_NEIGHBORS 16
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);
LIST(neighbors_list);


/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast");
PROCESS(example_unicast_process, "Unicast");
PROCESS(PDR_timer, "PDR timer");
AUTOSTART_PROCESSES(&example_broadcast_process, &example_unicast_process, &PDR_timer);
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
         
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(PDR_timer, ev, data)
{

  struct neighbor *n;
  static struct etimer et; 
  
  PROCESS_BEGIN();
  
  while(1) {
    /* Delay process for an hour and a bit(longest node synch time) */
    etimer_set(&et, CLOCK_SECOND * 3800);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
      printf("Address: %u.%u Packet delivery ratio: %u%\n", n->addr.u8[0], n->addr.u8[1], (unsigned int)(n->received/0.12));
    }
    n->received = 0;
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
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  struct neighbor *n;
  
  printf("unicast message received from %d.%d, message: '%s'\n",
	 from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
	 
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {

    /* We break out of the loop if the address of the neighbor matches
       the address of the neighbor from which we received this
       broadcast message. */
    if(linkaddr_cmp(&n->addr, from)) {
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

    /* Initialize the fields. */
    linkaddr_copy(&n->addr, from);
    n->received = 0;

    /* Place the neighbor on the neighbor list. */
    list_add(neighbors_list, n);
  }
  n->received = n->received++;
  printf("Received data: %u\n", n->received);
  
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;

PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)
    
  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
