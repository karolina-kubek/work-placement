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
broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
}
/*---------------------------------------------------------------------------*/
static void
unicast_recv(struct unicast_conn *c, const linkaddr_t *from) {
  struct neighbor *n;
  linkaddr_t *addr;
  printf("Sink: Table received from: %d.%d\n",
         from->u8[0], from->u8[1]);
  for(n = packetbuf_dataptr(); n->addr.u8[0] != 0; n = n + 1) {
    linkaddr_copy(addr, &n->addr);
    printf("Sink: Addr: %u.%u Devider: %u, RSSI: %u, LQI: %u\n", addr->u8[0], addr->u8[1], n->divider, n->last_rssi, n->last_lqi);
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
