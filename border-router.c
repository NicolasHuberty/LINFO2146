#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include "utils.h"
//Add node value in  message to know who we are talking about

#define CLOCK_REQUEST_INTERVAL (5 * CLOCK_SECOND)
#define SEND_INTERVAL (1 * CLOCK_SECOND)
#define ALIVE_INTERVAL (1*CLOCK_SECOND)
#define MAX_COORDINATORS 10
#define TIME_SLOT_DURATION CLOCK_SECOND // Set the duration of the time slot

#define HELLO_TYPE 1
#define ASK_CLOCK_TYPE 2
#define GIVE_CLOCK_TYPE 3
#define SET_CLOCK_TYPE 4 //rssi used to store the time slot
void berkeley_algorithm();
struct coordinator_info {
  linkaddr_t addr;
  clock_time_t clock_value;
  clock_time_t time_slot_start;
};
static struct etimer periodic_timer;
static struct etimer clock_request_timer;
static struct etimer alive_timer;
static clock_time_t clock_offset = 0;

struct coordinator_info coordinators[MAX_COORDINATORS];
uint8_t num_coordinators = 0;

void remove_non_responsive_coordinators() {
  uint8_t i = 0;
  while (i < num_coordinators) {
    if (coordinators[i].clock_value == 0) {
      printf("Coordinator %d.%d didn't respond, removing...\n", coordinators[i].addr.u8[0], coordinators[i].addr.u8[1]);

      // Shift the remaining coordinators to fill the gap
      for (uint8_t j = i; j < num_coordinators - 1; j++) {
        coordinators[j] = coordinators[j + 1];
      }
      num_coordinators--;
    } else {
      // Reset clock_value for the next iteration
      coordinators[i].clock_value = 0;
      i++;
    }
  }

  printf("Call berkeley algorithm from remove\n");
  berkeley_algorithm();
  
}

void send_ask_clock_to_all_coordinators() {
  printf("In send ask clock to all\n");
    for (uint8_t i = 0; i < num_coordinators; i++) {
      create_unicast_message(coordinators[i].addr, packetbuf_attr(PACKETBUF_ATTR_RSSI), 3, ASK_CLOCK_TYPE, 00);
    }
}

/*---------------------------------------------------------------------------*/
PROCESS(border_router_node_process, "Border Router");
AUTOSTART_PROCESSES(&border_router_node_process);

void handle_clock_request(struct message *clock_req_msg);
void berkeley_algorithm();
void send_data_request();

/*---------------------------------------------------------------------------*/

void send_clock_request() {
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 3,ASK_CLOCK_TYPE, 0);
  etimer_set(&alive_timer,ALIVE_INTERVAL);
}
void set_clock_time(){
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI),3,SET_CLOCK_TYPE,clock_time());
}
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  // Check if the message length is correct
  if(len != sizeof(struct message)) {
    printf("Invalid message length: %d (expected %d)\n", (int)len, (int)sizeof(struct message));
  }

  // Cast the message payload to a struct message pointer
  struct message *msg = (struct message*) data;

  // Check if the message is received during the assigned time slot
  // for (uint8_t i = 0; i < num_coordinators; i++) {
  //   if (linkaddr_cmp(src, &coordinators[i].addr)) {
  //     clock_time_t current_time = clock_time();
  //     if (current_time < coordinators[i].time_slot_start || current_time >= coordinators[i].time_slot_start + TIME_SLOT_DURATION) {
  //       printf("Message received outside the assigned time slot from coordinator %d.%d, ignoring...\n", src->u8[0], src->u8[1]);
  //       return;
  //     }
  //     break;
  //   }
  // }
  printf("RECEIVE A MESSAGE");
  // Process the message based on its type
  if(msg->type == HELLO_TYPE){
    linkaddr_copy(&coordinators[num_coordinators].addr, src);
      coordinators[num_coordinators].time_slot_start = TIME_SLOT_DURATION * num_coordinators;
      coordinators[num_coordinators].clock_value = clock_time(); //Initiate at own clock time value
      num_coordinators++;
      printf("Received the address of a coordinator, total: %d\n", num_coordinators);
  }
  if(msg->type == GIVE_CLOCK_TYPE){
    printf("To handle");
     for (uint8_t i = 0; i < num_coordinators; i++) {
        if (linkaddr_cmp(src, &coordinators[i].addr)) {
          coordinators[i].clock_value = msg->data;
          printf("Received clock value %lu from coordinator %d.%d\n", (unsigned long)msg->data, src->u8[0], src->u8[1]);
          break;
        }
      }

  }

}

void handle_clock_request(struct message *clock_req_msg) {
  // Send the current clock value as a response
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 3,3, clock_time());
}



void berkeley_algorithm() {
  // Calculate the average clock value
  clock_time_t sum = clock_time() + clock_offset;
  for (uint8_t i = 0; i < num_coordinators; i++) {
    sum += coordinators[i].clock_value;
  }
  clock_time_t average = sum / (num_coordinators + 1);

  // Adjust local clock offset
  clock_offset = average - clock_time();

  // Send adjusted clock value to coordinators
  for (uint8_t i = 0; i < num_coordinators; i++) {
    printf("Try to send to %d.%d\n", coordinators[i].addr.u8[0], coordinators[i].addr.u8[1]);   
    create_unicast_message(coordinators[i].addr,packetbuf_attr(PACKETBUF_ATTR_RSSI),3,4,average);
    // struct message *msg;
    // msg = (struct message*) malloc(sizeof(struct message));

    // //creation
    // msg->rssi = 0;
    // msg->nodeType = 3;
    // msg->type = 4;
    // msg->data = average;

    // // Set nullnet buffer and length
    // nullnet_buf = (uint8_t *)msg;
    // nullnet_len = sizeof(struct message);
    // NETSTACK_NETWORK.output(&coordinators[i].addr);
    // free(msg);
  }
  etimer_set(&alive_timer,CLOCK_REQUEST_INTERVAL+CLOCK_SECOND);
  printf("Time synchronized with Berkeley algorithm, new clock offset: %ld\n", (long)clock_offset);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_node_process, ev, data)
{


PROCESS_BEGIN();

nullnet_set_input_callback(input_callback);
create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 3,1, 00);
etimer_set(&periodic_timer, SEND_INTERVAL);
etimer_set(&clock_request_timer, CLOCK_REQUEST_INTERVAL);
etimer_set(&alive_timer,CLOCK_REQUEST_INTERVAL+CLOCK_SECOND);


while (1) {
  PROCESS_WAIT_EVENT();

  if (etimer_expired(&alive_timer)) {
    //Do nothing
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
