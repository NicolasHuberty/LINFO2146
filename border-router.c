#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include "utils.h"

#define CLOCK_REQUEST_INTERVAL (5 * CLOCK_SECOND)
#define SEND_INTERVAL (1 * CLOCK_SECOND)
#define ALIVE_INTERVAL (1*CLOCK_SECOND)
#define TIME_SLOT_DURATION CLOCK_SECOND // Set the duration of the time slot
#define WINDOW 10


static struct etimer periodic_timer;
static struct etimer clock_request_timer;
static struct etimer alive_timer;
static clock_time_t clock_offset = 0;
clock_time_t clock_request_send_time;

struct coordinator_info coordinators[2];
uint8_t num_coordinators = 0;

struct sensor_info sensors_info[10];
int nb_total_sensors = 0;

void berkeley_algorithm();
void send_data_request();

PROCESS(border_router_node_process, "Border Router");
AUTOSTART_PROCESSES(&border_router_node_process);

//Check if all coordinators has answer and call berkeley_algorithm
void remove_non_responsive_coordinators() {
  berkeley_algorithm();
  uint8_t i = 0;
  while (i < num_coordinators) {
    if (coordinators[i].clock_value == 0) {
      printf("Coordinator %d%d didn't respond, removing...\n", coordinators[i].addr.u8[0], coordinators[i].addr.u8[1]);
      for (uint8_t j = i; j < num_coordinators - 1; j++) {
        coordinators[j] = coordinators[j + 1];
      }
      num_coordinators--;
    } else {
      coordinators[i].clock_value = 0;
      i++;
    }
  }
}

//This function asks the clock to all coordinators
void send_ask_clock_to_all_coordinators() {
    for (uint8_t i = 0; i < num_coordinators; i++) {
      create_unicast_message(coordinators[i].addr, packetbuf_attr(PACKETBUF_ATTR_RSSI), BORDER_ROUTER, ASK_CLOCK_TYPE, 00);
    }
    clock_request_send_time = clock_time();
}


void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  if (len == sizeof(struct message)){
      struct message *msg = (struct message*) data;

      if(msg->type == HELLO_TYPE && msg->nodeType == COORDINATOR){ //HELLO message from coordinator
          linkaddr_copy(&coordinators[num_coordinators].addr, src);
          //coordinators[num_coordinators].time_slot_start = TIME_SLOT_DURATION * num_coordinators;
          //coordinators[num_coordinators].clock_value = clock_time(); //Initiate at own clock time value
          num_coordinators++;
          printf("Received the address of a coordinator, total: %d\n", num_coordinators);
      }

      if(msg->type == GIVE_CLOCK_TYPE){
        for (uint8_t i = 0; i < num_coordinators; i++) {
          if (linkaddr_cmp(src, &coordinators[i].addr)) {
            clock_time_t time_diff = clock_time() - clock_request_send_time;
            coordinators[i].clock_value = msg->data + time_diff;
            printf("Received clock value %d adjusted by %d ms, border router time: %d from coordinator %d :  %d.%d\n", (int) msg->data, (int) time_diff, (int) clock_time(), i, src->u8[0], src->u8[1]);
            break;
          }
        }
      }
    }

    if(len == sizeof(struct message_array_data)){
      struct message_array_data *msg = (struct message_array_data*) data;

      for(int i = 0; i < num_coordinators; i++){
        if(linkaddr_cmp(&(coordinators[i].addr),(src))){ //Check wich coordinator contacts us
          if(coordinators[i].time_slot_start > clock_time() && coordinators[i].time_slot_start < clock_time() + WINDOW/num_coordinators){ //Check if he sends data on his timeslot
            coordinators[i].nb_sensors = msg->nb_sensors;
            //coordinators[i].sensors = msg->sensors; TODO
          }
          printf("Border router treated received data\n");
        }
      }
  }

}

void berkeley_algorithm() {
  // Calculate the average clock value
  clock_time_t sum = 0;
  for (uint8_t i = 0; i < num_coordinators; i++) {
    sum += coordinators[i].clock_value;
  }
  sum += clock_time(); // Add border router's own clock value
  clock_time_t average = sum / (num_coordinators + 1);
  clock_offset = average - clock_time(); //TODO Local changes

  // Send adjusted clock value to coordinators
  for (uint8_t i = 0; i < num_coordinators; i++) {
    coordinators[i].clock_value =  clock_offset;
    coordinators[i].time_slot_start = clock_offset + ((WINDOW / num_coordinators) * i);
    create_unicast_clock_update(coordinators[i].addr, coordinators[i].clock_value,coordinators[i].time_slot_start,WINDOW, WINDOW / num_coordinators);
  }

  etimer_set(&alive_timer, CLOCK_REQUEST_INTERVAL + CLOCK_SECOND);
  printf("Time synchronized with Berkeley algorithm, new clock offset: %ld and new clock expected: %ld and was before: %ld coordinator time:%ld\n", clock_offset, clock_time() + clock_offset, clock_time(), coordinators[0].clock_value);
}


PROCESS_THREAD(border_router_node_process, ev, data){
PROCESS_BEGIN();
nullnet_set_input_callback(input_callback);
create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), BORDER_ROUTER, HELLO_TYPE, 00);
etimer_set(&periodic_timer, SEND_INTERVAL);
etimer_set(&clock_request_timer, CLOCK_REQUEST_INTERVAL);
etimer_set(&alive_timer,CLOCK_REQUEST_INTERVAL+CLOCK_SECOND);

while (1) {
  PROCESS_WAIT_EVENT();

  if (etimer_expired(&alive_timer)) {
    remove_non_responsive_coordinators();
  }

  if (etimer_expired(&clock_request_timer)) {
    // Request clock times from all coordinators every 5 seconds    
    send_ask_clock_to_all_coordinators();
    etimer_reset(&clock_request_timer);
  }
}
PROCESS_END();
}
