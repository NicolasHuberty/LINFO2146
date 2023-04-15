#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "random.h"
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
static int best_rssi = -999;
uint16_t values[3];   
static int random_value = 0;

/*---------------------------------------------------------------------------*/
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
  if(len == sizeof(unsigned)) {
    unsigned received_value;
    memcpy(&received_value, data, sizeof(received_value));
    LOG_INFO("Received value %u from ", received_value);
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_process, ev, data) {

  static struct etimer periodic_timer;
  static unsigned count = 0;

  PROCESS_BEGIN();

  #if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
  #endif /* MAC_CONF_WITH_TSCH */

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&random_value;
  nullnet_len = sizeof(random_value);
  nullnet_set_input_callback(input_callback);

  /* Add random parent candidates to the array since we chose to go with only 3 for now */
  linkaddr_t parent1 = {{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}; // for testing I am setting this to the same addr of the coordinator
  linkaddr_t parent2 = {{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
  linkaddr_t parent3 = {{ 0x04, 0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00 }};

  /* Add each parent candidate to the array of potential parents */
  parent_candidates[0] = parent1;
  parent_candidates[1] = parent2;
  parent_candidates[2] = parent3;

   while(1) {   
    /* Select the parent among the candidates */
    linkaddr_t best_parent = {{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
    int rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

    for(int i = 0; i < NUM_PARENT_CANDIDATES; i++) {
      linkaddr_t current_parent = parent_candidates[i];
      if(linkaddr_cmp(&current_parent, &coordinator_addr)) {
      /* Select the coordinator as the parent if it is among the candidates */
      best_parent = current_parent;
      break;
      } else {
        if(rssi >= best_rssi) {
        LOG_INFO("Best rssi %d \n", rssi);
          best_rssi = rssi;
          //see why the best parent is not being taken
          best_parent = current_parent;
          break;
        }
      }
    }

    /* Generate random sensor data */
      for(int j = 0; j < 3; j++) {
        values[j] = (random_rand() % 100) + 50;
      }

    /* Check if the parent address is not null */
    if(!linkaddr_cmp(&best_parent, &linkaddr_null)) {
      /* Set the destination address to the selected parent */
      memcpy(&dest_addr, &best_parent, sizeof(linkaddr_t));
      LOG_INFO("Selected parent with address: ");
      LOG_INFO_LLADDR(&dest_addr);
      LOG_INFO_("\n");

      /* Send generated random data to the parent */
      random_value = values[random_rand() % 3];
    LOG_INFO("Sending value %d to parent addr: ", random_value);
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