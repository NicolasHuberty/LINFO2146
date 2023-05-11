#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include "utils.h"
#define CLOCK_REQUEST_INTERVAL (5 * CLOCK_SECOND)
#define WINDOW 1000

//TODO -> Implement alive sensors with the data set to 0


//Custom functions to modify the clock_time value calling custom_clock_time()
static clock_time_t custom_clock_offset = 0;
void set_custom_clock_offset(clock_time_t offset)
{
  custom_clock_offset = offset;
}
clock_time_t custom_clock_time()
{
  return clock_time() - custom_clock_offset;
}

static struct etimer clock_request_timer;
static struct etimer alive_timer;
clock_time_t clock_request_send_time;

struct coordinator_info coordinators[10];
uint8_t num_coordinators = 0;

struct sensor_info sensors_info[5];

void berkeley_algorithm();
void send_data_request();

PROCESS(border_router_node_process, "Border Router");
AUTOSTART_PROCESSES(&border_router_node_process);

void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  if (len == sizeof(struct message)){
      struct message *msg = (struct message*) data;

      if(msg->type == HELLO_TYPE && msg->nodeType == COORDINATOR){ //HELLO message from coordinator
          printf("Received the address of a coordinator, total: %d\n", num_coordinators);
      
          for(int i = 0; i < num_coordinators;i++){
            if(linkaddr_cmp(src,&coordinators[i].addr)){return;}
          }
          linkaddr_copy(&coordinators[num_coordinators].addr, src);
          coordinators[num_coordinators].nb_sensors = 0;

          //Send the coordinator id to the coordinator in HELLO message
          create_unicast_message(coordinators[num_coordinators].addr,packetbuf_attr(PACKETBUF_ATTR_RSSI),BORDER_ROUTER,HELLO_TYPE,(int)num_coordinators);
          num_coordinators++;
          
      }

      if(msg->type == GIVE_CLOCK_TYPE && msg->nodeType == COORDINATOR){ // Receive clock of one coordinator
        for (uint8_t i = 0; i < num_coordinators; i++) {
          if (linkaddr_cmp(src, &coordinators[i].addr)) {
            //Adapt the coordinator clock val to its offset with Border custom clock
            coordinators[i].clock_value = msg->data + custom_clock_time();
            //printf("Receive clock value %d ,border router time: %d from coordinator %d :  %d.%d\n", (int) coordinators[i].clock_value,(int) custom_clock_time(), i, src->u8[0], src->u8[1]);
            break;
          }
        }
      }
    }
    if(len == sizeof(struct message_data)){ //Receive a message data forwarded by a coordinator
    struct message_data *msg = (struct message_data *)data;
      for(int i = 0; i < num_coordinators; i++){
        if(linkaddr_cmp(&(coordinators[i].addr),(src))){ //Check wich coordinator contacts us
          bool found = false;
          for(int j = 0; j < coordinators[i].nb_sensors;j++){ //Check if sensor is found in the coordinator list
            if(linkaddr_cmp(&msg->addr,&coordinators[i].sensors[j].addr)){
              found = true;
              coordinators[i].sensors[j].data = msg->data; //Update the sensor;
            }
          }
            if(coordinators[i].nb_sensors == 0 || !found){ //Add a new sensor
              coordinators[i].sensors[0].addr = msg->addr;
              coordinators[i].sensors[0].data = msg->data;
              coordinators[i].nb_sensors+=1;
            }
        }
      }
  }

}
//Check if all coordinators have answer and call berkeley_algorithm
void remove_non_responsive_coordinators() {
  berkeley_algorithm();
  uint8_t i = 0;
  while (i < num_coordinators) {
    if (coordinators[i].clock_value == 0) {
      printf("Coordinator %d%d didn't respond, removing...\n", coordinators[i].addr.u8[0], coordinators[i].addr.u8[1]);
      for (uint8_t j = i; j < num_coordinators - 1; j++) {
        coordinators[j] = coordinators[j + 1];
        // Resend HELLO message to update the identifier of all coordinators
        create_unicast_message(coordinators[j].addr,packetbuf_attr(PACKETBUF_ATTR_RSSI),BORDER_ROUTER,HELLO_TYPE,(int)j);
      }
      num_coordinators--;
    } else {
      //Reset clock_value to check in the next call if the clock_value has been modify
      //clock_value is used to detect dead coordinators
      coordinators[i].clock_value = 0;
      i++;
    }
  }
}

void send_ask_clock_to_all_coordinators() {
    for (uint8_t i = 0; i < num_coordinators; i++) {
      create_unicast_message(coordinators[i].addr, packetbuf_attr(PACKETBUF_ATTR_RSSI), BORDER_ROUTER, ASK_CLOCK_TYPE, 00);
    }
    clock_request_send_time = custom_clock_time();
    etimer_set(&clock_request_timer,CLOCK_REQUEST_INTERVAL);
}

void berkeley_algorithm() {
  // Calculate the average clock value
  clock_time_t sum = 0;
  for (uint8_t i = 0; i < num_coordinators; i++) {
    sum += coordinators[i].clock_value;
  }
  sum += custom_clock_time();
  clock_time_t average = sum / (num_coordinators + 1);
  set_custom_clock_offset(average - clock_time());
  create_multicast_clock_update(average,WINDOW,num_coordinators);
  printf("Time synchronized with Berkeley algorithm and new clock expected: %ld and was before: %ld coordinator time:%ld\n", average, custom_clock_time(), coordinators[0].clock_value);
}


PROCESS_THREAD(border_router_node_process, ev, data){
  PROCESS_BEGIN();
  nullnet_set_input_callback(input_callback);
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), BORDER_ROUTER, HELLO_TYPE, 00);
  etimer_set(&clock_request_timer, CLOCK_REQUEST_INTERVAL);
  etimer_set(&alive_timer,CLOCK_REQUEST_INTERVAL+CLOCK_SECOND);

  while (1) {
    PROCESS_WAIT_EVENT();

    if(etimer_expired(&alive_timer)) { //All coordinators are expected to have responded
      remove_non_responsive_coordinators();
      etimer_set(&alive_timer,1000000000);
    }

    if (etimer_expired(&clock_request_timer)) {
      // Request clock times for all coordinators every 5 seconds    
      send_ask_clock_to_all_coordinators();
      etimer_set(&alive_timer,CLOCK_SECOND);
      etimer_reset(&clock_request_timer);
    }
  }
  PROCESS_END();
}