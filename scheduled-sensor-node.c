#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include <stdio.h>
#include "node-id.h"
/*---------------------------------------------------------------------------*/
PROCESS(example_broadcast_process, "Broadcast");
PROCESS(example_unicast_process, "Unicast");
AUTOSTART_PROCESSES(&example_broadcast_process,
                    &example_unicast_process);
/*---------------------------------------------------------------------------*/
static uint8_t time_stamp = 0;
static linkaddr_t sink_addr;
static uint8_t reply_flag = 0;
static void
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  printf("broadcast message received from %d.%d: %u\n",
         from->u8[0], from->u8[1], *(unsigned int *) packetbuf_dataptr());
         
  time_stamp = *(unsigned int *) packetbuf_dataptr();
  sink_addr = *from;
  reply_flag = 1;
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_broadcast_process, ev, data)
{
  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  broadcast_open(&broadcast, 129, &broadcast_call);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

static void
sent_uc(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d\n",
    dest->u8[0], dest->u8[1]);
}
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("Unicast received!");
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(example_unicast_process, ev, data)
{
  static struct etimer et;
  static uint8_t delay;
 
  PROCESS_EXITHANDLER(unicast_close(&uc);)
    
  PROCESS_BEGIN(); // uint8_t flag = 0; switch (flag){

  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {

    delay = time_stamp + node_id * 10;
    etimer_set(&et, CLOCK_SECOND * delay);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et)); // flag =1 ; break; case 1: 
    if(reply_flag != 0) {
      
      packetbuf_copyfrom("Hello", 5);
      
      if(!linkaddr_cmp(&sink_addr, &linkaddr_node_addr)) {
      
        unicast_send(&uc, &sink_addr);
        printf("Unicast sent!\n");
        reply_flag = 0;
        
      }
      
    }
    etimer_stop(&et);

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
