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
#define MAX_NODES 10

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static linkaddr_t coord_node = {{0, 0}};
static linkaddr_t sensor_node = {{0, 0}};

struct coord_info {
  linkaddr_t addr; // Parent node address
  int rssi; // RSSI value of the last received message from this coord
};

struct coord_info coords[MAX_NODES];
linkaddr_t coord_list[MAX_NODES];

struct sensor_information {
  linkaddr_t addr; // Parent node address
  int rssi; // RSSI value of the last received message from this coord
};

struct sensor_information sensors[MAX_NODES];
linkaddr_t sensor_list[MAX_NODES];
int num_coords = 0;
int num_sensors = 0;

clock_time_t last_coord_message_time = 0;
clock_time_t last_sensor_message_time = 0;

/*---------------------------------------------------------------------------*/
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

void choose_coord();
void send_message(int rssi, int nodeType, int type, clock_time_t clock_value);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
  
  // Check if the message length is correct
  if(len != sizeof(struct message)) {
    printf("Invalid message length: %d (expected %d)\n", (int)len,(int) sizeof(struct message));
    return;
  }

  struct message *msg = (struct message*) data;
  
  // Check if the message is from the coord node
  if(linkaddr_cmp(&coord_node, src)) {
    // Update the time the last message was received from the coord node
    last_coord_message_time = clock_time();

    // Print a message indicating that the coord node is still alive
    printf("Received message from coord node %d.%d with RSSI %d\n", src->u8[0], src->u8[1], msg->rssi);
  } else if(linkaddr_cmp(&sensor_node, src)) {
    // Update the time the last message was received from the coord node
    last_sensor_message_time = clock_time();

    // Print a message indicating that the coord node is still alive
    printf("Received message from sensor node %d.%d with RSSI %d\n", src->u8[0], src->u8[1], msg->rssi);
  }

  // Check if the coord node has sent a message within the last 30 seconds
  if(coord_node.u8[0] != 0 && clock_time() - last_coord_message_time > CLOCK_SECOND * 30) {
    // Remove the coord node from the list of coords
    num_coords = 0;
    coord_node.u8[0] = 0;
    coord_node.u8[1] = 0;
    printf("Coord node %d.%d is no longer alive, it has been removed from the list\n", src->u8[0], src->u8[1]);
  } else if(sensor_node.u8[0] != 0 && clock_time() - last_sensor_message_time > CLOCK_SECOND * 30) {
    // Remove the coord node from the list of coords
    num_sensors = 0;
    sensor_node.u8[0] = 0;
    sensor_node.u8[1] = 0;
    printf("Sensor node %d.%d is no longer alive, it has been removed from the list\n", src->u8[0], src->u8[1]);
  }

  if(msg->type == RESPONSE_HELLO_MSG){
    // Check if this coord is already in the list
    int coord_index = -1;
    for(int i=0; i<num_coords; i++){
      if(linkaddr_cmp(&coord_list[i], src)){
        coord_index = i;
        break;
      }
    }

    // Add the new coord to the list
    if(coord_index == -1){
      coord_list[num_coords] = *src;
      coords[num_coords].addr = *src; 
      coords[num_coords].rssi = msg->rssi;
      num_coords += 1;
      printf("Coordinator node: %d.%d has been added to the list \n", src->u8[0], src->u8[1]);
      printf("Number of coords: %d\n", num_coords);

    } else {
      // Update the RSSI value for an existing coord
      coords[coord_index].rssi = msg->rssi;
    }
    
  } else if(msg->type == HELLO_TYPE) {
     int sensor_index = -1;
    for(int i=0; i<num_sensors; i++){
      if(linkaddr_cmp(&sensor_list[i], src)){
        sensor_index = i;
        break;
      }
    }
    
	if(sensor_index == -1){
      sensor_list[num_sensors] = *src;
      sensors[num_sensors].addr = *src; 
      sensors[num_sensors].rssi = msg->rssi;
      num_sensors += 1;
      printf("Sensor node: %d.%d has been added to the list \n", src->u8[0], src->u8[1]);
      printf("Number of sensors: %d\n", num_sensors);

    } else {
      // Update the RSSI value for an existing coord
      sensors[sensor_index].rssi = msg->rssi;
    }
  }
  
  //Used for the border router
  if(msg->type == ALLOW_SEND_DATA && msg->nodeType == 2){
      /* Generate and send random data */
      int random_value = (random_rand() % 100) + 50;
      create_unicast_message_data(*src, DATA, random_value);
      printf("Sent %d to coord\n", random_value);
    } 
}

/*---------------------------------------------------------------------------*/
void choose_coord() {
  // Choose coord with highest RSSI value
  int best_rssi_coord = -100;
  int best_rssi_index_coord = -1;
  
  int best_rssi_sensor = -100;
  int best_rssi_index_sensor = -1;

  for(int i = 0; i < num_coords; i++) {
  if(coords[i].rssi > best_rssi_coord) {
      best_rssi_coord = coords[i].rssi;
      best_rssi_index_coord = i;
    }
  }
  
  for(int i = 0; i < num_sensors; i++) {
  if(sensors[i].rssi > best_rssi_sensor) {
      best_rssi_sensor = sensors[i].rssi;
      best_rssi_index_sensor = i;
    }
  }

if(num_coords > 0 && best_rssi_coord != -100){
  if(best_rssi_index_coord != -1) {
    // Set selected coord as coord
    coord_node = coords[best_rssi_index_coord].addr;
    printf("Selected parent is coord with addr: %d.%d\n", coord_node.u8[0], coord_node.u8[1]);
    create_unicast_message(coord_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), 0, CHOSEN_PARENT, 0);
    
  } else {
    coord_node.u8[0] = 0;
    coord_node.u8[1] = 0;
  	printf("No candidates, parent addr is: %d.%d\n", coord_node.u8[0], coord_node.u8[1]);
  }
 } else if(num_sensors > 0) {
  	if(best_rssi_index_sensor != -1) {
    // Set selected parent as sensor
    sensor_node = sensors[best_rssi_index_sensor].addr;
    printf("Selected parent is sensor with addr: %d.%d\n", sensor_node.u8[0], sensor_node.u8[1]);
    create_unicast_message(sensor_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), 0, CHOSEN_PARENT, 0);
    
  } else {
    sensor_node.u8[0] = 0;
    sensor_node.u8[1] = 0;
  	printf("No candidates, sensor parent addr is: %d.%d\n", sensor_node.u8[0], sensor_node.u8[1]);
	 }
	}
}

void send_message(int rssi, int nodeType, int type, clock_time_t clock_value){
  choose_coord();
  
  // Broadcast: Send message to all nodes in the network
  create_multicast_message(rssi, nodeType, HELLO_TYPE, clock_value);
  if(coord_node.u8[0] != 0) {
    // Unicast: Send message to the parent
    create_unicast_message(coord_node, rssi, nodeType, HELLO_TYPE, clock_value);
    printf("Sensor is sending to its COORD parent with addr: %d.%d\n", coord_node.u8[0], coord_node.u8[1]);
  } else if(sensor_node.u8[0] != 0) {
    // Unicast: Send message to the parent
    create_unicast_message(sensor_node, rssi, 0, HELLO_TYPE, clock_value);
    printf("Sensor is sending to its SENSOR parent with addr: %d.%d\n", sensor_node.u8[0], sensor_node.u8[1]);
  } 
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_process, ev, data) {
  static struct etimer periodic_timer;
  
  PROCESS_BEGIN();

  nullnet_set_input_callback(input_callback);

   while(1) {      
    send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 0, HELLO_TYPE, 00);
    
    /* Wait for the next transmission interval */
    etimer_set(&periodic_timer, SEND_INTERVAL);        
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}