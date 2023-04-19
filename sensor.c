#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "random.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h> /* For printf() */

#include "sys/log.h"
#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SEND_INTERVAL (8 * CLOCK_SECOND)

static linkaddr_t parent_node = {{0, 0}};
uint16_t values[3];   
static int random_value = 0;

int best_rssi = -999;

struct message {
 int rssi;
 int nodeType;
 int data;
};

/*---------------------------------------------------------------------------*/
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
  if(len == sizeof(struct message)) {
  // Cast the message payload to a struct message pointer
  struct message *msg = (struct message*) data;
  printf("received %d from ", msg->data);
  if(msg->nodeType == 0) {
    printf("SENSOR ");
  } else {
    printf("COORDINATOR ");
  } 
  printf("node %d.%d with RSSI %d \n", src->u8[0], src->u8[1], msg->rssi);
  
  if(msg->data == 42){
    parent_node = *src; // Set parent_node to current node
    printf("chose a coordinator with address %d.%d as its parent\n", src->u8[0], src->u8[1]);
  }
  } else {
  printf("Invalid message length: %d (expected %d)\n", len, sizeof(struct message));
  }
}
/*---------------------------------------------------------------------------*/
void create_message(int rssi, int nodeType, int data){
 
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //msg creation
  msg->rssi=rssi;
  msg->nodeType=nodeType;
  msg->data=data;
  
  printf("Parent node: %d.%d\n", parent_node.u8[0], parent_node.u8[1]);

  LOG_INFO("OLD rssi %d\n", best_rssi);
  if(rssi > best_rssi){ 
    best_rssi = rssi;
    LOG_INFO("NEW rssi %d\n", best_rssi);
  }
  
   // Send message to all nodes in the network
  if(parent_node.u8[0] == 0) {
    uint8_t len = sizeof(struct message);
    void *payload = (void*)msg;
    memcpy(nullnet_buf, payload, len);
    nullnet_len = len;
    NETSTACK_NETWORK.output(NULL);
  
    printf("Sensor Sent Message to all nodes\n");
  }
  // Send message only to the selected parent
  else {
    uint8_t len = sizeof(struct message);
    void *payload = (void*)msg;
    memcpy(nullnet_buf, payload, len);
    nullnet_len = len;
    NETSTACK_NETWORK.output(&parent_node);
  
    printf("Sensor Sent Message to parent node \n");
    }

  free(msg);
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(sensor_node_process, ev, data) {

  static struct etimer periodic_timer;
  static unsigned count = 0;
  int rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  PROCESS_BEGIN();

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&random_value;
  nullnet_len = sizeof(random_value);
  nullnet_set_input_callback(input_callback);

   while(1) {
 

  /* Generate random sensor data */
  for(int j = 0; j < 3; j++) {
    values[j] = (random_rand() % 100) + 50;
  }

  /* Send random generated data */
  random_value = values[random_rand() % 3];
    create_message(-(rssi), 0, random_value);

  count++;

    /* Wait for the next transmission interval */
    etimer_set(&periodic_timer, SEND_INTERVAL);        
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
