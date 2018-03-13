#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>
#include "node-id.h"


/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast");
AUTOSTART_PROCESSES(&example_broadcast_process);
/*---------------------------------------------------------------------------*/
static uint8_t time_stamp = 0;
static uint8_t reply_flag = 0;
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  if( *(unsigned int *) packetbuf_dataptr() != 0 ) {
    printf("broadcast message received from %d.%d: %u\n",
         from->u8[0], from->u8[1], *(unsigned int *) packetbuf_dataptr());
    printf("sent packet %u\n",
             packetbuf_attr(PACKETBUF_ATTR_PACKET_ID));    
    time_stamp = *(unsigned int *) packetbuf_dataptr();
    reply_flag = 1;
  }
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  static struct etimer et, xt;
  static uint8_t delay;
  static uint8_t introduction = 0;
  
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    
  PROCESS_BEGIN(); // uint8_t flag = 0; switch (flag){

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
    
    etimer_stop(&et);
  }
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
