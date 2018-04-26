#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>
#include <string.h>
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#include "node-id.h"

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

#define MAX_NEIGHBORS 16
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);
LIST(neighbors_list);

static uint8_t flag = 0;
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast");
PROCESS(retransmission_PDR_timer, "Retransmission PDR timer");
AUTOSTART_PROCESSES(&example_broadcast_process, &retransmission_PDR_timer);

/*---------------------------------------------------------------------------*/
static uint8_t time_stamp = 0;
static uint8_t reply_flag = 0;
static uint8_t table_flag = 0;
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  // Table forwarding request from sink  
  if (*(unsigned int *) packetbuf_dataptr() == 1 && from->u8[0] == 1 ) {
      printf("Request to forward table received from %d.%d: %u\n",
         from->u8[0], from->u8[1], *(unsigned int *) packetbuf_dataptr());
    table_flag = 1;
    
  // Introduction request from sink + timestamp
  } else if( *(unsigned int *) packetbuf_dataptr() != 0 && from->u8[0] == 1 ) {
    printf("Request to intoduce received from %d.%d: %u\n",
         from->u8[0], from->u8[1], *(unsigned int *) packetbuf_dataptr());  
    time_stamp = *(unsigned int *) packetbuf_dataptr();
    reply_flag = 1;
    
  // Sensor introduction message
  } else if ( *(unsigned int *) packetbuf_dataptr() == 0 && from->u8[0] != 1 ) {
    uint8_t neighbour_id;     
    struct neighbor *n;  
    printf("Introduction message received from %d.%d: %u\n",
	 from->u8[0], from->u8[1], *(unsigned int *) packetbuf_dataptr());

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
      n->id = neighbour_id;
      n->sno1 = 0;
      n->sno2 = 0;
      n->sno3 = 0;
      n->PDR_avg = 0;
      n->divider = 0;

    /* Place the neighbor on the neighbor list. */
      list_add(neighbors_list, n);
    } 
    
    n->last_rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    n->last_lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);
  
    if( n->sno1 == 0 ) {
      n->sno1 = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    } else if( n->sno2 == 0 ) {
      n->sno2 = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    } else if( n->sno3 == 0 ) {
      n->sno3 = packetbuf_attr(PACKETBUF_ATTR_PACKET_ID);
    } else {
      printf("Number out of range!\n");
    }
    flag = neighbour_id;
  }
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et, xt, kt;
  struct neighbor *n;
  static uint8_t delay;
  static uint8_t introduction = 0;
  
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    
  PROCESS_BEGIN(); // uint8_t flag = 0; switch (flag){
  
  /* We assign the first position on the neighbor list to be the node itself */
  n = memb_alloc(&neighbors_memb);
  n->addr.u8[0] = linkaddr_node_addr.u8[0];
  n->addr.u8[1] = linkaddr_node_addr.u8[1];
  list_add(neighbors_list, n);
  
  broadcast_open(&broadcast, 129, &broadcast_call);

  while(1) {

    etimer_set(&xt, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&xt));
    
    if(reply_flag != 0) {
      delay = time_stamp + node_id * 10;
      etimer_set(&et, CLOCK_SECOND * delay);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); // flag =1 ; break; case 1: 
      packetbuf_copyfrom(&introduction, sizeof(uint8_t));
      broadcast_send(&broadcast);
      printf("1st Data broadcasted!\n");
      packetbuf_copyfrom(&introduction, sizeof(uint8_t));
      broadcast_send(&broadcast);
      printf("2nd Data broadcasted!\n");
      packetbuf_copyfrom(&introduction, sizeof(uint8_t));
      broadcast_send(&broadcast);
      printf("3rd Data broadcasted!\n");
      reply_flag = 0;  
    } 
    if ( table_flag != 0 ) {
      printf("Neighbour table sent by broadcast!\n");
      etimer_set(&kt, CLOCK_SECOND * delay);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&kt)); 
      packetbuf_copyfrom(list_head(neighbors_list), sizeof(*n)*6);
      broadcast_send(&broadcast);
      printf("Neighbour table sent by broadcast!\n");
      table_flag = 0;
    }
    
    etimer_stop(&et);
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
    
    for(n = list_item_next(list_head(neighbors_list)); n != NULL; n = list_item_next(n)) {
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
