#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "net/routing/routing.h"
#include "random.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "sys/log.h"

#include "utils.h"

/* Configuration */
#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO
#define MAX_NODES 10

#define SEND_INTERVAL (1 * CLOCK_SECOND)

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

struct sensor_information sensors_list[MAX_NODES];
linkaddr_t sensor_list[MAX_NODES];
int num_coords = 0;
int num_sensors = 0;

clock_time_t last_coord_message_time = 0;
clock_time_t last_sensor_message_time = 0;

static clock_time_t previous_message_time = 0;

// Choose coord with highest RSSI value/
static int best_rssi_coord = -100;
static int best_rssi_index_coord = -1;

// Choose sensor with highest RSSI value/
static int best_rssi_sensor = -100;
static int best_rssi_index_sensor = -1;

static struct etimer alive_timer;

/*---------------------------------------------------------------------------*/
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

void choose_parent();

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
  
  // Check if the message length is correct

  if(len != sizeof(struct message)) {
    printf("Invalid message length: %d (expected %d)\n", (int)len,(int) sizeof(struct message));
    return;
  }

  struct message *msg = (struct message*) data;
	  if(msg->type == RESPONSE_HELLO_MSG){
	  	printf("Received message from coord node %d.%d with RSSI %d\n", src->u8[0], src->u8[1], msg->rssi);
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
		choose_parent();	
  	}
	  

 if(msg->type == HELLO_TYPE && msg->nodeType == COORDINATOR) {
 	printf("sent unicast\n");
  	create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, HELLO_TYPE, 0);
 }

 if(msg->type == HELLO_TYPE && msg->nodeType == SENSOR) {
 	printf("Received message from sensor node %d.%d with RSSI %d\n", src->u8[0], src->u8[1], msg->rssi);
		
    int sensor_index = -1;
    for(int i=0; i<num_sensors; i++){
      if(linkaddr_cmp(&sensor_list[i], src)){
        sensor_index = i;
        break;
      }
    }
    
	if(sensor_index == -1){
      sensor_list[num_sensors] = *src;
      sensors_list[num_sensors].addr = *src; 
      sensors_list[num_sensors].rssi = msg->rssi;
      num_sensors += 1;
      printf("Sensor node: %d.%d has been added to the list \n", src->u8[0], src->u8[1]);
      printf("Number of sensors: %d\n", num_sensors);

    } else {
      // Update the RSSI value for an existing coord
      sensors_list[sensor_index].rssi = msg->rssi;
    }
  }
    
    if(msg->type == ALLOW_SEND_DATA && msg->nodeType == COORDINATOR && linkaddr_cmp(&coord_node, src)){
		printf("Received allow_send_data, timer: %d \n", (int)(2*(clock_time()-previous_message_time)));
		etimer_set(&alive_timer, CLOCK_SECOND);
		previous_message_time = clock_time();
		
		// Generate and send random data
	 	 int random_value = (random_rand() % 100) + 50;
	 	 create_unicast_message_data(*dest, *src, DATA, random_value);
	 	 printf("Sent %d to coord\n", random_value);
		}
}

/*---------------------------------------------------------------------------*/
void choose_parent() {
	int previous_rssi = best_rssi_coord;
	printf("Entered choose_parent, Number of coords: %d\n", num_coords);

  for(int i = 0; i < num_coords; i++) {
  if(coords[i].rssi > best_rssi_coord) {
      best_rssi_coord = coords[i].rssi;
      best_rssi_index_coord = i;
    }
  }
  
  for(int i = 0; i < num_sensors; i++) {
  if(sensors_list[i].rssi > best_rssi_sensor) {
      best_rssi_sensor = sensors_list[i].rssi;
      best_rssi_index_sensor = i;
    }
  }
 
if(previous_rssi != best_rssi_coord && previous_rssi != -100){
	printf("Sensor sent a REMOVE_PARENT message\n");
	create_unicast_message(coord_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, REMOVE_PARENT, 0);
}


if(num_coords > 0 && best_rssi_coord != -100){
  if(best_rssi_index_coord != -1) {
	// Set selected coord as coord
	coord_node = coords[best_rssi_index_coord].addr;
	printf("Selected parent is coord with addr: %d.%d\n", coord_node.u8[0], coord_node.u8[1]);
	create_unicast_message(coord_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, CHOSEN_PARENT, 0);
	
  } else {
	coord_node.u8[0] = 0;
	coord_node.u8[1] = 0;
  	printf("No candidates, parent addr is: %d.%d\n", coord_node.u8[0], coord_node.u8[1]);
  }

 } else if(num_sensors > 0) {
  	if(best_rssi_index_sensor != -1) {
    // Set selected parent as sensor
    sensor_node = sensors_list[best_rssi_index_sensor].addr;
    printf("Selected parent is sensor with addr: %d.%d\n", sensor_node.u8[0], sensor_node.u8[1]);
    create_unicast_message(sensor_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, CHOSEN_PARENT, 0);
    
  } else {
    sensor_node.u8[0] = 0;
    sensor_node.u8[1] = 0;
  	printf("No candidates, sensor parent addr is: %d.%d\n", sensor_node.u8[0], sensor_node.u8[1]);
	 }
   }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_process, ev, data) {
  
  PROCESS_BEGIN();

  nullnet_set_input_callback(input_callback);
  
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, HELLO_TYPE, clock_time());

  while(1) {  
 	if(etimer_expired(&alive_timer)){
 		etimer_set(&alive_timer, 5*CLOCK_SECOND);
		coords[best_rssi_index_coord].rssi = -100;
		printf("RESET RSSI\n");
 		choose_parent();
 	}
  
    /* Wait for the next transmission interval */
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
