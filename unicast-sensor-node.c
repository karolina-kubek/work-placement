#include "contiki.h"
#include "net/rime/rime.h"
#include <stdio.h>
#include "lib/random.h"
#include <math.h>

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Example unicast");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/

static uint8_t flag = 0;

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  printf("End unicast message received from %d.%d\n",
	 from->u8[0], from->u8[1]);
  if(*(uint8_t *)packetbuf_dataptr() == 4){
    printf("Break!\n");
    flag = 1;
  }
}
/*---------------------------------------------------------------------------*/
static void
sent_uc(struct unicast_conn *c, int status, int num_tx)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  printf("unicast message sent to %d.%d: status %d num_tx %d\n",
    dest->u8[0], dest->u8[1], status, num_tx);
}
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc, sent_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)
    
  PROCESS_BEGIN();

  unicast_open(&uc, 146, &unicast_callbacks);

  while(1) {
    static struct etimer et;
    static uint8_t value;
    linkaddr_t addr;
    
    /*Etimer set to dely between 10 and 30 sec*/
    etimer_set(&et, CLOCK_SECOND * 10 + random_rand() % (CLOCK_SECOND * 20));
    
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    value = (random_rand() % 9) + 1;
    packetbuf_copyfrom(&value, 8);
    addr.u8[0] = 1;
    addr.u8[1] = 0;
    if(!linkaddr_cmp(&addr, &linkaddr_node_addr) && flag == 0) {
      unicast_send(&uc, &addr);
    }

  }

  PROCESS_END();
}
