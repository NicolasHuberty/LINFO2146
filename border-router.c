#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include "utils.h"
#include "dev/serial-line.h"

#define CLOCK_REQUEST_INTERVAL (5 * CLOCK_SECOND)
#define WINDOW 5000

static clock_time_t custom_clock_offset = 0;
static struct etimer clock_request_timer;
static struct etimer alive_timer;
clock_time_t clock_request_send_time;
uint8_t num_coordinators = 0;

/*Set an offset to modify the clock_time of the mote*/
void set_custom_clock_offset(clock_time_t offset){
  custom_clock_offset = offset;
}

/*Get the adapted clock_time*/
clock_time_t custom_clock_time(){
  return clock_time() - custom_clock_offset;
}

struct coordinator_info coordinators[10];
struct sensor_info sensors_info[5];

void berkeley_algorithm();
void send_data_request();

/*---------------------------------------------------------------------------*/
PROCESS(border_router_node_process, "Border Router");
AUTOSTART_PROCESSES(&border_router_node_process);

/*---------------------------------------------------------------------------*/

void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest){
  /*Receive a network message packet*/
  if (len == sizeof(struct message)){
    struct message *msg = (struct message*) data;
    /*Discover a new coordinator*/
    if(msg->type == HELLO_TYPE && msg->nodeType == COORDINATOR){ 
        //printf("Received the address of a coordinator, total: %d\n", num_coordinators);
        for(int i = 0; i < num_coordinators;i++){
          if(linkaddr_cmp(src,&coordinators[i].addr)){return;}
        }
        linkaddr_copy(&coordinators[num_coordinators].addr, src);
        coordinators[num_coordinators].nb_sensors = 0;
        coordinators[num_coordinators].clock_value = 1;
        create_unicast_message(coordinators[num_coordinators].addr,packetbuf_attr(PACKETBUF_ATTR_RSSI),BORDER_ROUTER,HELLO_TYPE,(int)num_coordinators);
        num_coordinators++; 
    }
    /*Receive the clock_time of a coordinator to calculate the Berkeley algorithm*/
    if(msg->type == GIVE_CLOCK_TYPE && msg->nodeType == COORDINATOR){
      for (uint8_t i = 0; i < num_coordinators; i++) {
        if (linkaddr_cmp(src, &coordinators[i].addr)) {
          coordinators[i].clock_value = msg->data + custom_clock_time();
          //printf("Receive clock value %d ,border router time: %d from coordinator %d :  %d.%d\n", (int) coordinators[i].clock_value,(int) custom_clock_time(), i, src->u8[0], src->u8[1]);
          break;
        }
      }
    }
  }
    /*Receive a data from a coordinator*/
    if(len == sizeof(struct message_data)){ 
      struct message_data *msg = (struct message_data *)data;
      for(int i = 0; i < num_coordinators; i++){
        /*Check on the list of coordinator*/
        if(linkaddr_cmp(&(coordinators[i].addr),(src))){
          coordinators[i].clock_value = 1;
          bool found = false;
          for(int j = 0; j < coordinators[i].nb_sensors;j++){
            if(linkaddr_cmp(&msg->addr,&coordinators[i].sensors[j].addr)){
              found = true;
              /*Check if the sensor has to be remove*/
              if (msg->data == -1){
                printf("Delete sensor %d \n",coordinators[i].sensors[j].addr.u8[0]);
                for(int k = j;k < coordinators[i].nb_sensors -1;k++){
                  coordinators[i].sensors[j] = coordinators[i].sensors[j+1];
                }
                coordinators[i].nb_sensors -= 1;
              }
              else{
                coordinators[i].sensors[j].data = msg->data; //Update the sensor;
                printf("Receive %d from sensor %d.%d\n", msg->data, coordinators[i].sensors[j].addr.u8[0], coordinators[i].sensors[j].addr.u8[1]);
              }              
            }
          }
          /*Receive data from a new sensor*/
          if(!found){
            coordinators[i].sensors[coordinators[i].nb_sensors].addr = msg->addr;
            coordinators[i].sensors[coordinators[i].nb_sensors].data = msg->data;
            coordinators[i].nb_sensors+=1;
          }
        }
      }
    }
}

/*Remove lost coordinators*/
void remove_non_responsive_coordinators() {
  berkeley_algorithm();
  uint8_t i = 0;
  while (i < num_coordinators) {
    if (coordinators[i].clock_value == 0) {
      printf("Coordinator %d%d didn't respond, removing...\n", coordinators[i].addr.u8[0], coordinators[i].addr.u8[1]);
      for (uint8_t j = i; j < num_coordinators - 1; j++) {
        coordinators[j] = coordinators[j + 1];
        /*Update the identifier of all coordinators*/
        create_unicast_message(coordinators[j].addr,packetbuf_attr(PACKETBUF_ATTR_RSSI),BORDER_ROUTER,HELLO_TYPE,(int)j);
      }
      num_coordinators--;
    }else {
      /*Set clock_value to detect dead coordinators*/
      coordinators[i++].clock_value = 0;
    }
  }
}

/*Send unicast message to all coordinators to get their clock_time */
void send_ask_clock_to_all_coordinators() {
    for (uint8_t i = 0; i < num_coordinators; i++) {
      create_unicast_message(coordinators[i].addr, packetbuf_attr(PACKETBUF_ATTR_RSSI), BORDER_ROUTER, ASK_CLOCK_TYPE,0);
    }
    clock_request_send_time = custom_clock_time();
    /*Let CLOCK_REQUEST_INTERVAL seconds to coordinators to answer before removing them*/
    etimer_set(&clock_request_timer,CLOCK_REQUEST_INTERVAL);
}

/*Algorithm to synchronize all coordinators and border router*/
void berkeley_algorithm() {
  clock_time_t sum = 0;
  for (uint8_t i = 0; i < num_coordinators; i++) {
    sum += coordinators[i].clock_value;
  }
  sum += custom_clock_time();
  clock_time_t average = sum / (num_coordinators + 1);
  set_custom_clock_offset(average - clock_time());
  create_multicast_clock_update(average,WINDOW,num_coordinators);
  //printf("Time synchronized with Berkeley algorithm and new clock expected: %ld and was before: %ld coordinator time:%ld\n", average, custom_clock_time(), coordinators[0].clock_value);
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(border_router_node_process, ev, data){
  PROCESS_BEGIN();
  nullnet_set_input_callback(input_callback);
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), BORDER_ROUTER, HELLO_TYPE, 0);
  etimer_set(&clock_request_timer, CLOCK_REQUEST_INTERVAL);
  etimer_set(&alive_timer,CLOCK_REQUEST_INTERVAL+CLOCK_SECOND);
  serial_line_init();
  while (1) {
    PROCESS_WAIT_EVENT();
    /*Timer to launch berkeley algorithm and remove unresponded coordinators*/
    if(etimer_expired(&alive_timer)){
      remove_non_responsive_coordinators();
      etimer_set(&alive_timer,100000*CLOCK_SECOND); //Disable the timer
    }
    /*Ask to all coordinators their clock_time*/
    if (etimer_expired(&clock_request_timer)) {
      send_ask_clock_to_all_coordinators();
      //Let one second to all coordinators to answer before removing them
      etimer_set(&alive_timer,CLOCK_SECOND);
      etimer_reset(&clock_request_timer);
    }
  }
  PROCESS_END();
}