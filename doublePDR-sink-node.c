#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>

struct neighbor {
  struct neighbor *next;
  linkaddr_t addr;
  uint8_t id;
  uint8_t received;
  uint8_t sno[3];
};

uint8_t PDR_avg = 100;
uint8_t divider = 1;
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
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  uint8_t neighbour_id;     
  struct neighbor *n;  
  printf("broadcast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);
  printf("sent packet %u \n",
             packetbuf_attr(PACKETBUF_ATTR_PACKET_ID));

  neighbour_id = 1;	 
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {

    /* We break out of the loop if the address of the neighbor matches
       the address of the neighbor from which we received this
       broadcast message. */
    if(linkaddr_cmp(&n->addr, from)) {
      break;
    }
    neighbour_id++;
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
    n->id = neighbour_id;
    n->sno[0] = 0;
    n->sno[1] = 0;
    n->sno[2] = 0;

    /* Place the neighbor on the neighbor list. */
    list_add(neighbors_list, n);
  }
  n->received = n->received++;
  if( n->sno[0] == 0 ) {
    n->sno[0] = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    printf("Position 0 occupied: %u!\n", n->sno[0]);
  } else if( n->sno[1] == 0 ) {
    n->sno[1] = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
        printf("Position 1 occupied: %u!\n", n->sno[1]);
  } else if( n->sno[2] == 0 ) {
    n->sno[2] = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
        printf("Position 2 occupied: %u!\n", n->sno[2]);
  } else {
    printf("Number out of range!\n");
  }
  flag = neighbour_id;
  printf("Received data: %u\n", n->received);
 
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
    
    for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
      printf("Address: %u.%u Packet delivery ratio: %u%\n", n->addr.u8[0], n->addr.u8[1], (unsigned int)(n->received/0.36));
      n->received = 0;
    }
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(retransmission_PDR_timer, ev, data)
{

  struct neighbor *n;
  static struct etimer et, xt; 
  static uint8_t avg = 0;
  PROCESS_BEGIN();
  
  while(1) {
    etimer_set(&et, CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    
    for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {
      
      if(n->id == flag) {
        printf("Id: %u, flag: %u, addr: %u.%u\n", n->id, flag, n->addr.u8[0], n->addr.u8[1]);
        etimer_set(&xt, CLOCK_SECOND * 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&xt));
        printf("First sqno: %u, second: %u, third: %u\n", n->sno[0], n->sno[1], n->sno[2]);
        
        if( (((n->sno[0])+1) == n->sno[1]) || (((n->sno[1])+1) == n->sno[2]) || (((n->sno[0])+2) == n->sno[2]) ){
          avg = 66;
          printf("Retransmission avg: 66%\n");
        } else if( (((n->sno[0])+1) == n->sno[1]) && (((n->sno[1])+1) == n->sno[2]) && (((n->sno[0])+2) == n->sno[2]) ){
          avg = 100;
          printf("Retransmission avg: 100%\n");
        } else {
          avg = 33;
          printf("Retransmission avg: 33%\n");
        }
        
        PDR_avg = (uint8_t)(( (PDR_avg * divider) + avg ) / divider+1);
        divider = divider + 1;
        printf("Retransmission PDR: %u Divider: %u Node: %u.%u\n", PDR_avg, divider, n->addr.u8[0], n->addr.u8[1]);
        flag = 0;
        
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
