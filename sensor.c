#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "net/routing/routing.h"
#include "random.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h> /* For printf() */
#include "sys/log.h"

#include "utils.h"

#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO
#define MAX_PARENTS 10

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static linkaddr_t parent_node = {{0, 0}};

struct parent_info {
  linkaddr_t addr; // Parent node address
  int rssi; // RSSI value of the last received message from this parent
};

struct parent_info parents[MAX_PARENTS];
linkaddr_t parent_list[MAX_PARENTS];
int num_parents = 0;

clock_time_t last_parent_message_time = 0;

/*---------------------------------------------------------------------------*/
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

void choose_parent();
void send_message(int rssi, int nodeType, int type, clock_time_t clock_value);



/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
  
  // Check if the message length is correct
  if(len != sizeof(struct message)) {
    printf("Invalid message length: %d (expected %d)\n", (int)len,(int) sizeof(struct message));
    return;
  }

  struct message *msg = (struct message*) data;
  
  printf("received %d from ", msg->data);
  if(msg->nodeType == 0) {
    printf("SENSOR ");
  } else {
    printf("COORDINATOR ");
  } 
  printf("node %d.%d with RSSI %d \n", src->u8[0], src->u8[1], msg->rssi);
  
  // Check if the message is from the parent node
  if(linkaddr_cmp(&parent_node, src)) {
    // Update the time the last message was received from the parent node
    last_parent_message_time = clock_time();

    // Print a message indicating that the parent node is still alive
    printf("Received message from parent node %d.%d\n", src->u8[0], src->u8[1]);
  }

  // Check if the parent node has sent a message within the last 30 seconds
  if(parent_node.u8[0] != 0 && clock_time() - last_parent_message_time > CLOCK_SECOND * 30) {
    // Remove the parent node from the list of parents
    num_parents = 0;
    parent_node.u8[0] = 0;
    parent_node.u8[1] = 0;
    printf("Parent node %d.%d is no longer alive, it has been removed from the list\n", src->u8[0], src->u8[1]);
  }

  if(msg->data == 42){
    // Check if this parent is already in the list
    int parent_index = -1;
    for(int i=0; i<num_parents; i++){
      if(linkaddr_cmp(&parent_list[i], src)){
        parent_index = i;
        break;
      }
    }

    // Add the new parent to the list
    if(parent_index == -1){
      parent_list[num_parents] = *src;
      parents[num_parents].addr = *src; 
      parents[num_parents].rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
      num_parents += 1;
      printf("The coordinator node: %d.%d has been added to the list \n", src->u8[0], src->u8[1]);
      printf("Number of parents: %d\n", num_parents);

    } else {
      // Update the RSSI value for an existing parent
      parents[parent_index].rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
    }
  } 
}

/*---------------------------------------------------------------------------*/
void choose_parent() {
  // Choose parent with highest RSSI value
  int best_rssi = -100;
  int best_rssi_index = -1;
  for(int i = 0; i < num_parents; i++) {
  if(parents[i].rssi > best_rssi) {
      best_rssi = parents[i].rssi;
      best_rssi_index = i;
    }
  }

    printf("Number of parents: %d\n", num_parents);
  if(best_rssi_index != -1 && num_parents > 0) {
    // Set selected parent
    parent_node = parents[best_rssi_index].addr;
    printf("Selected parent: %d.%d\n", parent_node.u8[0], parent_node.u8[1]);
  } else {
    parent_node.u8[0] = 0;
    parent_node.u8[1] = 0;
  	printf("No candidats left, parent is back to default: %d.%d\n", parent_node.u8[0], parent_node.u8[1]);
  }
}

//Add a way to check if the parent is still alive

void send_message(int rssi, int nodeType, int type, clock_time_t clock_value){
 
  choose_parent();
  
   // Broadcast: Send message to all nodes in the network
  if(parent_node.u8[0] == 0) {
    create_multicast_message(rssi, nodeType, 1, clock_value);  
    printf("No parent yet, sensor broadcasted messages to all nodes\n");
  }
  // Unicast: Send message only to the selected parent
  else {
    create_unicast_message(parent_node, rssi, nodeType, 1, clock_value);
    printf("Sensor is sending unicast messages to its Parent with addr: %d.%d\n", parent_node.u8[0], parent_node.u8[1]);
  } 
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_process, ev, data) {
  static struct etimer periodic_timer;
  
  PROCESS_BEGIN();

  nullnet_set_input_callback(input_callback);

   while(1) {
     /* Generate random sensor data */
    int random_value = (random_rand() % 100) + 50;
      
    /* Send random generated data (unicast or multicast) */
    send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 0, random_value, 00);
    
    /* Wait for the next transmission interval */
    etimer_set(&periodic_timer, SEND_INTERVAL);        
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
