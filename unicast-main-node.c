#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "net/rime/rime.h"
#include <math.h>
#include <stdio.h>
#include "cc2420.h"

/* This structure holds information about neighbors. */
struct neighbor {

  struct neighbor *next;
  linkaddr_t addr;
  uint8_t values[5];

};

/* This #define defines the maximum amount of neighbors we can remember. */
#define MAX_NEIGHBORS 20

/* This MEMB() definition defines a memory pool from which we allocate
   neighbor entries. */
MEMB(neighbors_memb, struct neighbor, MAX_NEIGHBORS);

/* The neighbors_list is a Contiki list that holds the neighbors we
   have seen thus far. */
LIST(neighbors_list);


/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Example unicast");
AUTOSTART_PROCESSES(&example_unicast_process);
/*---------------------------------------------------------------------------*/

static void
recv_uc(struct unicast_conn *c, const linkaddr_t *from)
{
  struct neighbor *n;
  uint8_t i;
  /* Check if we already know this neighbor. */
  for(n = list_head(neighbors_list); n != NULL; n = list_item_next(n)) {

    /* We break out of the loop if the address of the neighbor matches
       the address of the neighbor from which we received this
       broadcast message. */
    if(linkaddr_cmp(&n->addr, from)) {
      
      /* Assign values to the array */
      for(i = 1; i < 5; i++) {
        if(n->values[i] == 0) {
          n->values[i] = *(uint8_t *)packetbuf_dataptr();
      	  printf("Stored number: %u, on position: %u for: %d.%d\n", n->values[i], i,  from->u8[0], from->u8[1]);
      	  
      	  /* Send a message that the pool is now full */
      	  if(i == 4) {
      	    printf("Pool is now full for %d.%d\n", from->u8[0], from->u8[1]);
      	    packetbuf_copyfrom(&i, 8);
      	    
            /* Send it back to where it came from. */
            unicast_send(c, from);
      	  }
          break;
        }
      }
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
    n->values[0] = *(uint8_t *)packetbuf_dataptr();
    printf("Stored number: %u, on position: 0\n", n->values[0]);

    /* Place the neighbor on the neighbor list. */
    list_add(neighbors_list, n);
  }

  printf("unicast message received from %d.%d Number : %u\n",
	 from->u8[0], from->u8[1], n->values[0]);
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks unicast_callbacks = {recv_uc};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&uc);)
  
  PROCESS_BEGIN();
  unicast_open(&uc, 146, &unicast_callbacks);
  
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
