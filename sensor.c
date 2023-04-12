#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include <string.h>
#include <stdio.h> /* For printf() */

#include "sys/log.h"
#define LOG_MODULE "Sensor Node"
#define LOG_LEVEL LOG_LEVEL_INFO

#define NUM_PARENT_CANDIDATES 3
#define SEND_INTERVAL (8 * CLOCK_SECOND)

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
#endif /* MAC_CONF_WITH_TSCH */

static linkaddr_t parent_candidates[NUM_PARENT_CANDIDATES];
static linkaddr_t coordinator_addr = {{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
static linkaddr_t dest_addr;
static unsigned count = 0;

/*---------------------------------------------------------------------------*/
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
  if(len == sizeof(unsigned)) {
    unsigned received_count;
    memcpy(&received_count, data, sizeof(received_count));
    LOG_INFO("Received %u from ", received_count);
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_process, ev, data) {

  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  #if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
  #endif /* MAC_CONF_WITH_TSCH */

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
  nullnet_set_input_callback(input_callback);

  /* Add random parent candidates to the array since we chose to go with only 3 for now */
  parent_candidates[0] = {{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}; // for testing I am setting this to the same addr of the coordinator
  parent_candidates[1] = {{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
  parent_candidates[2] = {{ 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

   while(1) {
    /* Select the parent among the candidates */
    linkaddr_t best_parent = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
    for(int i = 0; i < NUM_PARENT_CANDIDATES; i++) {
      linkaddr_t current_parent = parent_candidates[i];
      if(linkaddr_cmp(&current_parent, &coordinator_addr)) {
        /* Select the coordinator as the parent if it is among the candidates */
        best_parent = current_parent;
        break;
      }
    }

    /* Check if the parent address is not null */
    if(!linkaddr_cmp(&best_parent, &linkaddr_null)) {
      /* Set the destination address to the selected parent */
      memcpy(&dest_addr, &best_parent, sizeof(linkaddr_t));
      LOG_INFO("Selected parent with address: ");
      LOG_INFO_LLADDR(&dest_addr);
      LOG_INFO_("\n");

      /* Send hello to the parent */
      LOG_INFO("Sending HELLO %u to: ", count);
      LOG_INFO_LLADDR(&dest_addr);
      LOG_INFO_("\n");
      NETSTACK_NETWORK.output(&dest_addr);
      count++;
    }

    /* Wait for the next transmission interval */
    etimer_set(&periodic_timer, SEND_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}