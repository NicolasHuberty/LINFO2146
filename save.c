#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 

#define CLOCK_REQUEST_INTERVAL (5 * CLOCK_SECOND)
#define SEND_INTERVAL (1 * CLOCK_SECOND)
#define MAX_COORDINATORS 10
#define TIME_SLOT_DURATION CLOCK_SECOND // Set the duration of the time slot

struct coordinator_info {
  linkaddr_t addr;
  clock_time_t clock_value;
  clock_time_t time_slot_start;
};

struct coordinator_info coordinators[MAX_COORDINATORS];
uint8_t num_coordinators = 0;

struct message {
  int rssi;
  int nodeType;
  int data;
  clock_time_t clock_value;
  clock_time_t time_slot; // Add time slot field
};

#define MSG_TYPE_HELLO 1
#define MSG_TYPE_CLOCK_REQUEST 2
#define MSG_TYPE_CLOCK_RESPONSE 3
#define MSG_TYPE_DATA 4

/*---------------------------------------------------------------------------*/
PROCESS(border_router_node_process, "Border Router");
AUTOSTART_PROCESSES(&border_router_node_process);

void handle_clock_request(struct message *clock_req_msg);
void berkeley_algorithm();
void send_data_request();

/*---------------------------------------------------------------------------*/
void create_send_message(int rssi, int eachNodeType, int data, clock_time_t clock_value, clock_time_t time_slot) {
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //creation
  msg->rssi = rssi;
  msg->nodeType = eachNodeType;
  msg->data = data;
  msg->clock_value = clock_value;
  msg->time_slot = time_slot; // Add time slot field

  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message);

  //sending
  NETSTACK_NETWORK.output(NULL);

  free(msg);
}

void send_clock_request() {
  create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), MSG_TYPE_CLOCK_REQUEST, 0, 0, 0);
}
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  // Check if the message length is correct
  if(len != sizeof(struct message)) {
    printf("Invalid message length: %d (expected %d)\n", (int)len, (int)sizeof(struct message));
  }
  printf("Received message from node %d.%d:\n", src->u8[0], src->u8[1]);
  printf("RSSI: %d\n", msg->rssi);
  printf("eachNodeType: %d\n", msg->eachNodeType);
  printf("data: %d\n", msg->data);

  // Cast the message payload to a struct message pointer
  struct message *msg = (struct message*) data;

  // Check if the message is received during the assigned time slot
  for (uint8_t i = 0; i < num_coordinators; i++) {
    if (linkaddr_cmp(src, &coordinators[i].addr)) {
      clock_time_t current_time = clock_time();
      if (current_time < coordinators[i].time_slot_start || current_time >= coordinators[i].time_slot_start + TIME_SLOT_DURATION) {
        printf("Message received outside the assigned time slot from coordinator %d.%d, ignoring...\n", src->u8[0], src->u8[1]);
        return;
      }
      break;
    }
  }
  printf("RECEIVE A MESSAGE");
  // Process the message based on its type
  switch (msg->nodeType) {
    case MSG_TYPE_HELLO:
      if (msg->data == 42 && num_coordinators < MAX_COORDINATORS) {
        linkaddr_copy(&coordinators[num_coordinators].addr, src);
        coordinators[num_coordinators].time_slot_start = TIME_SLOT_DURATION * num_coordinators;
        num_coordinators++;
        printf("Received the address of a coordinator, total: %d\n", num_coordinators);
        send_clock_request(); // Send a clock request to all coordinators after receiving a hello message
      }
      break;

    case MSG_TYPE_CLOCK_RESPONSE:
      for (uint8_t i = 0; i < num_coordinators; i++) {
        if (linkaddr_cmp(src, &coordinators[i].addr)) {
          coordinators[i].clock_value = msg->clock_value;
          printf("Received clock value %lu from coordinator %d.%d\n", (unsigned long)msg->clock_value, src->u8[0], src->u8[1]);
          break;
        }
      }

      // Check if all coordinators have responded
      uint8_t all_responded = 1;
      for (uint8_t i = 0; i < num_coordinators; i++) {
        if (coordinators[i].clock_value == 0) {
          all_responded = 0;
          break;
        }
      }

      if (all_responded) {
        berkeley_algorithm();
      }
      break;

    case MSG_TYPE_DATA:
      // Process data message
      printf("Received data message from coordinator %d.%d: %d\n", src->u8[0], src->u8[1], msg->data);
      break;

    default:
      printf("Unknown message type: %d\n", msg->nodeType);
      break;
  }
}

void handle_clock_request(struct message *clock_req_msg) {
  // Send the current clock value as a response
  create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), MSG_TYPE_CLOCK_RESPONSE, 0, clock_time(), 0);
}


void berkeley_algorithm() {
  // Calculate the average clock value
  clock_time_t sum = clock_time();
  for (uint8_t i = 0; i < num_coordinators; i++) {
    sum += coordinators[i].clock_value;
  }
  clock_time_t average = sum / (num_coordinators + 1);

  // Adjust local clock
  //clock_set(average);

  // Send adjusted clock value to coordinators
  for (uint8_t i = 0; i < num_coordinators; i++) {
    nullnet_buf = (uint8_t *)&average;
    nullnet_len = sizeof(clock_time_t);
    NETSTACK_NETWORK.output(&coordinators[i].addr);
  }

  printf("Time synchronized with Berkeley algorithm, new clock value: %lu\n", (unsigned long)average);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_node_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer clock_request_timer;

PROCESS_BEGIN();

nullnet_set_input_callback(input_callback);
create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 1, 21, 0, 0);
etimer_set(&periodic_timer, SEND_INTERVAL);
etimer_set(&clock_request_timer, CLOCK_REQUEST_INTERVAL);

while (1) {
  PROCESS_WAIT_EVENT();

  if (etimer_expired(&periodic_timer)) {
    create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 1, 21, 0, 0);
    etimer_reset(&periodic_timer);
  }

  if (etimer_expired(&clock_request_timer)) {
    // Request clock times from all coordinators every 5 seconds
    printf("Should be print all 5seconds");
    send_clock_request();
    etimer_reset(&clock_request_timer);
  }
}
PROCESS_END();
}
