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

#define LOG_MODULE "Sensor"
#define LOG_LEVEL LOG_LEVEL_INFO
#define MAX_NODES 10
#define SEND_INTERVAL (1 * CLOCK_SECOND)

struct coord_info {
  linkaddr_t addr; 
  int rssi;
};

struct sensor_information {
  linkaddr_t addr;
  int rssi; 
  int alive;
};

static struct etimer alive;
static clock_time_t previous_message_time = 0;
static linkaddr_t master_node = {{0,0}};
struct coord_info coords[MAX_NODES];
struct sensor_information sensors_list[MAX_NODES];
int num_coords = 0;
int num_sensors = 0;
static int best_rssi_coord = -100;
static int best_rssi_index_coord = -1;
static int best_rssi_sensor = -100;
static int best_rssi_index_sensor = -1;
void choose_parent();

<<<<<<< Updated upstream
static struct etimer alive;
static int parent = 0;
=======
>>>>>>> Stashed changes
/*---------------------------------------------------------------------------*/
PROCESS(sensor_node_process, "Sensor Node Process");
AUTOSTART_PROCESSES(&sensor_node_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) {
<<<<<<< Updated upstream
	struct message *msg = (struct message*) data;

	if(msg->type == HELLO_TYPE && msg->nodeType == COORDINATOR) { //Should be RESPOND_HELLO_TYPE BUT OK
		create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, HELLO_TYPE, 0);
		printf("sensor respont to hello message to coord\n");
	}
	

	if(msg->type == RESPONSE_HELLO_MSG && msg->nodeType == COORDINATOR){
  	printf("Received message from coord node %d.%d with RSSI %d\n", src->u8[0], src->u8[1], msg->rssi);
	// Check if this coord is already in the list
	int coord_index = -1;
	for(int i=0; i<num_coords; i++){
	  if(linkaddr_cmp(&coords[i].addr, src)){
	    coord_index = i;
	    break;
	  }
	}
	// Add the new coord to the list
	if(coord_index == -1){
	  coords[num_coords].addr = *src; 
	  coords[num_coords].rssi = msg->rssi;
	  num_coords++;
	  printf("Coordinator node: %d.%d has been added to the list of %d coordinators\n", src->u8[0], src->u8[1],num_coords);
	} else {
	  coords[coord_index].rssi = msg->rssi;
	}
	//Relaunch the choose Parent with the new mote added
	choose_parent();	
}
  
	if((msg->type == HELLO_TYPE && msg->nodeType == SENSOR) || (msg->type == RESPONSE_HELLO_MSG && msg->nodeType == SENSOR)) {
    if (msg->type == RESPONSE_HELLO_MSG && msg->nodeType == SENSOR){
      parent = 1;
      //printf("Variable parent : %d\n", parent);
    }  
	int sensor_index = -1;
	for(int i=0; i<num_sensors; i++){
	  if(linkaddr_cmp(&sensors_list[i].addr, src)){
	    sensor_index = i;
	    break;
	  }
	}
	if(sensor_index == -1){
	  sensors_list[num_sensors].addr = *src; 
	  sensors_list[num_sensors].rssi = msg->rssi;
	  sensors_list[num_sensors].alive = 3;
	  num_sensors ++;
	  printf("Sensor node: %d.%d has been added to the list of %d sensors\n", src->u8[0], src->u8[1],num_sensors);
	} else {
	  sensors_list[sensor_index].rssi = msg->rssi;
	}
  }

	if(msg->type == DATA) { //If sensor receives data and has a coordinator -> Forward data with NOT_MY_DATA
		//Reset alive values
		for(int i = 0; i < num_sensors; i++){
			if(linkaddr_cmp(src,&sensors_list[i].addr)){
				sensors_list[i].alive = 3;
=======
	/*Receive a network message packet*/
	if(len == sizeof(struct message)){
		struct message *msg = (struct message*) data;
		/*Discover a new coordinator*/
		if(msg->type == HELLO_TYPE && msg->nodeType == COORDINATOR) {
			create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, HELLO_TYPE, 0);
		}

		/*Add a new coordinator to its list of coordinator*/
		if(msg->type == RESPONSE_HELLO_MSG && msg->nodeType == COORDINATOR){
			int coord_index = -1;
			for(int i=0; i<num_coords; i++){
				if(linkaddr_cmp(&coords[i].addr, src)){
					coord_index = i;
					break;
				}
			}
		/*The coordinator is unknown, add it to the list of coordinator*/
		if(coord_index == -1){
			coords[num_coords].addr = *src; 
			coords[num_coords].rssi = msg->rssi;
			num_coords++;
			printf("Coordinator node: %d.%d has been added to the list of %d coordinators\n", src->u8[0], src->u8[1],num_coords);
		}else{
		coords[coord_index].rssi = msg->rssi;
		}
		/*Relaunch the choose parent with the new mote added*/
		choose_parent();	
	}
	/*Discover a new sensor*/
	if((msg->type == RESPONSE_HELLO_MSG && msg->nodeType == SENSOR)) {
		/*Detect if the sensor search a sensor as master node*/
		if(msg->data == 0){
			sensors_list[num_sensors].addr = *src; 
			sensors_list[num_sensors].rssi = msg->rssi;
			sensors_list[num_sensors].alive = 3;
			num_sensors ++;
			printf("Sensor node: %d.%d has been added to the list of %d sensors\n", src->u8[0], src->u8[1],num_sensors);
		}else{
			sensors_list[0].rssi = msg->rssi;
		}
	}
	/*Send NEED_PARENT message if sensor is searching for a master node*/
	if((msg->type == DO_YOU_NEED_PARENT && msg->nodeType == SENSOR)){
		if(num_coords <= 0){
			create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, RESPONSE_HELLO_MSG, (int)0);
		}
	}
	/*Discover new sensor*/
	if(msg->type == HELLO_TYPE && msg->nodeType == SENSOR) {
		if(num_coords > 0){
			create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, DO_YOU_NEED_PARENT,-1);
		}else{
			create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, RESPONSE_HELLO_MSG, (int)0);
		}
	}
	/*Coordinator allocates us the timeslot to send data*/
	if(msg->type == ALLOW_SEND_DATA && msg->nodeType == COORDINATOR ){
		if(timer_remaining(&alive.timer) < (int)(2*(clock_time()-previous_message_time))){
			//Berkeley algo just restart with significatif modifications
			etimer_set(&alive, (int)2*(clock_time()-previous_message_time));
		}
		previous_message_time = clock_time();
		// Generate and send random data
		int random_value = (random_rand() % 100) + 50;
		create_unicast_message_data(*src,master_node,DATA,random_value);
		//printf("Sensor sent: %d to coord\n", random_value);

		/*Sensor is master node of other sensors*/
		if(num_sensors > 0 && num_coords > 0){ 
			for(int i = 0; i < num_sensors; i++){
				sensors_list[i].alive--;
				create_unicast_message(sensors_list[i].addr,packetbuf_attr(PACKETBUF_ATTR_RSSI),COORDINATOR,ALLOW_SEND_DATA,0);
				//printf("Forwarded allow message from sensor with addr: %d.%d connected to me\n", sensors_list[i].addr.u8[0],sensors_list[i].addr.u8[1]);
>>>>>>> Stashed changes
			}
		}
		create_unicast_message(master_node,packetbuf_attr(PACKETBUF_ATTR_RSSI),SENSOR,NOT_MY_DATA,(int)msg->data);
		printf("Forwarded data %d received by a sensor child to coord with addr: %d.%d\n", (int)msg->data, master_node.u8[0], master_node.u8[1]);
	}
	if(msg->type == RESPONSE_HELLO_MSG && msg->nodeType == SENSOR){
		choose_parent();
	}
	if(msg->type == HELLO_TYPE && msg->nodeType == SENSOR) { //Send RESPONSE_HELLO_MSG
		printf("Receive HELLO MSG FROM OTHER SENSOR respond whith HELLO MESSAGE\n"); //Should add in the value 1 if connected to coordinator
		create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, RESPONSE_HELLO_MSG, 0);
		choose_parent();
	}
<<<<<<< Updated upstream

	if(msg->type == ALLOW_SEND_DATA && msg->nodeType == COORDINATOR ){
		//printf("Receive an ALLOW SEND DATA MESSAGE");
		for(int i = 0; i < num_sensors; i++){
			if(sensors_list[i].alive == 0){
				printf("Remove a sensor of the list\n");
      			for (uint8_t j = i; j < num_sensors - 1; j++) {
        			sensors_list[j] = sensors_list[j + 1];
      			}
				num_sensors --;
			}
=======
	}
	/*Receive a data message from another sensor*/
	if(len == sizeof(struct message_data)){
		struct message_data *msg = (struct message_data*) data;
		/*Receive a data message*/
		if(msg->type == DATA){ 
			for(int i = 0; i < num_sensors; i++){
				if(linkaddr_cmp(src,&sensors_list[i].addr)){
					sensors_list[i].alive = 3;
				}
			}
			create_unicast_message_data(master_node,*src,NOT_MY_DATA,(int)msg->data);
			//printf("Forwarded data %d received by a sensor child to coord with addr: %d.%d\n", (int)msg->data, master_node.u8[0], master_node.u8[1]);
>>>>>>> Stashed changes
		}
		if(timer_remaining(&alive.timer) < (int)(2*(clock_time()-previous_message_time))){
			//Berkeley algo does not restart
			etimer_set(&alive, (int)2*(clock_time()-previous_message_time));
		}
		previous_message_time = clock_time();

		// Generate and send random data
		int random_value = (random_rand() % 100) + 50;
		create_unicast_message_data(master_node, *src, DATA, random_value);	
		printf("Sent from coordinator: %d to coord\n", random_value);
    //printf("Variable parent : %d\n", parent);
    for(int i = 0; i < num_sensors; i++){
        if (num_sensors > 0 && num_coords > 0 && parent == 1){
				sensors_list[i].alive--;
				//create_unicast_message(sensors_list[i].addr,packetbuf_attr(PACKETBUF_ATTR_RSSI),COORDINATOR,ALLOW_SEND_DATA,0);
				create_unicast_message_data(sensors_list[i].addr, *src, NOT_MY_DATA, random_value);
				printf("Forwarded allow message from sensor with addr: %d.%d connected to me\n", sensors_list[i].addr.u8[0],sensors_list[i].addr.u8[1]);
        }  

			}
    }
	}

/*-----------------------------------------------SENSOR----------------------------*/
<<<<<<< Updated upstream
void choose_parent() {	
	printf("Come in choose parent\n");
=======
void choose_parent() {
	/*Search a master node*/	
	if(num_coords == 0 && num_sensors == 0){
  		create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, HELLO_TYPE, 0);
	}
>>>>>>> Stashed changes
	int previous_rssi = best_rssi_coord;
  	for(int i = 0; i < num_coords; i++) {
		if(coords[i].rssi > best_rssi_coord) {
			printf("Coord %d have a better rssi than: %d\n",i,best_rssi_index_coord);
			best_rssi_coord = coords[i].rssi;
			best_rssi_index_coord = i;
		}
<<<<<<< Updated upstream
  }
 
if(previous_rssi != best_rssi_coord && previous_rssi != -100){
	printf("Sensor sent a REMOVE_PARENT message\n");
	create_unicast_message(master_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, REMOVE_PARENT, 0);
}


if(num_coords > 0 && best_rssi_coord != -100){
  if(best_rssi_index_coord != -1) {
	// Set selected coord as coord
	master_node = coords[best_rssi_index_coord].addr;
	printf("Selected parent is coord with addr: %d.%d\n", master_node.u8[0], master_node.u8[1]);
	create_unicast_message(master_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, CHOSEN_PARENT, 0);
	etimer_set(&alive,10*CLOCK_SECOND);
  } else {
	master_node.u8[0] = 0;
	master_node.u8[1] = 0;
  	printf("No candidates, parent addr is: %d.%d\n", master_node.u8[0], master_node.u8[1]);
  }

 } else if(num_sensors > 0 && best_rssi_sensor != -100) {
  	if(best_rssi_index_sensor != -1) {
    // Set selected parent as sensor
    master_node = sensors_list[best_rssi_index_sensor].addr; // ATTENTION MODIFICATION IMPORTANTE
    printf("Selected parent is sensor (acting as coord) with addr: %d.%d\n", master_node.u8[0], master_node.u8[1]);
    //printf("Variable parent : %d\n", parent);
    create_unicast_message(master_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, CHOSEN_PARENT, 0);
    etimer_set(&alive,10*CLOCK_SECOND);
  } else {
    master_node.u8[0] = 0;
    master_node.u8[1] = 0;
  	printf("No candidates, sensor parent addr is: %d.%d\n", master_node.u8[0], master_node.u8[1]);
	 }
   }
   printf("Finish choose parent\n");
=======
  	}
  	for(int i = 0; i < num_sensors; i++) {
		if(sensors_list[i].rssi > best_rssi_sensor) {
			printf("Sensor %d have a better rssi than: %d\n",i,best_rssi_index_sensor);
			best_rssi_sensor = sensors_list[i].rssi;
			best_rssi_index_sensor = i;
		}
  	}
	/*Sensor sent REMOVE_PARENT when it discover beter master node*/
	if(previous_rssi != best_rssi_coord && previous_rssi != -100){
		printf("Sensor sent a REMOVE_PARENT message\n");
		create_unicast_message(master_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, REMOVE_PARENT, 0);
	}
	/*Discover a better parent in coordinator nodes*/
	if(num_coords > 0 && best_rssi_coord != -100){
		if(best_rssi_index_coord != -1) {
			master_node = coords[best_rssi_index_coord].addr;
			printf("Selected parent is coord with addr: %d.%d\n", master_node.u8[0], master_node.u8[1]);
			create_unicast_message(master_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, CHOSEN_PARENT, 0);
			etimer_set(&alive,10*CLOCK_SECOND);
		}else{
			master_node.u8[0] = 0;
			master_node.u8[1] = 0;
			printf("No candidates, parent addr is: %d.%d\n", master_node.u8[0], master_node.u8[1]);
		}
	/*Discover a better parent in sensor nodes*/
	}else if(num_sensors > 0 && best_rssi_sensor != -100) {
		if(best_rssi_index_sensor != -1) {
		master_node = sensors_list[best_rssi_index_sensor].addr; 
		printf("Selected parent is sensor (acting as coord) with addr: %d.%d\n", master_node.u8[0], master_node.u8[1]);
		create_unicast_message(master_node, packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, CHOSEN_PARENT, 0);
		etimer_set(&alive,10*CLOCK_SECOND);
	} else {
		master_node.u8[0] = 0;
		master_node.u8[1] = 0;
		printf("No candidates, sensor parent addr is: %d.%d\n", master_node.u8[0], master_node.u8[1]);
		}
	}
>>>>>>> Stashed changes
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sensor_node_process, ev, data) {
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  nullnet_set_input_callback(input_callback);
<<<<<<< Updated upstream
  
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, HELLO_TYPE, clock_time());
  printf("Sent hello Message to all\n");
=======
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), SENSOR, HELLO_TYPE, -1);
>>>>>>> Stashed changes
  etimer_set(&alive,20*CLOCK_SECOND);
  etimer_set(&periodic_timer,500);
  while(1) {  
    PROCESS_WAIT_EVENT();
    if(etimer_expired(&periodic_timer)){
      etimer_reset(&periodic_timer);
    }
	/*Try to take a better parent*/
    if(etimer_expired(&alive)){
      coords[best_rssi_index_coord].rssi = -100;
      best_rssi_coord = -1;
      if(num_coords > 0 ){
        num_coords--;
      }
      choose_parent();
      etimer_set(&alive,20*CLOCK_SECOND);
    }
  }
  PROCESS_END();
}
