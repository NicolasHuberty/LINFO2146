#include "utils.h"
#include <stdlib.h>
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"


void create_multicast_message(int rssi, int nodeType,int type,clock_time_t clock_value) {
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //creation
  msg->rssi = rssi;
  msg->nodeType = nodeType;
  msg->type = type;
  msg->data = clock_value;

  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message);

  //sending
  NETSTACK_NETWORK.output(NULL);

  free(msg);
}
void create_unicast_message(linkaddr_t addr,int rssi, int nodeType,int type,clock_time_t clock_value) {
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //creation
  msg->rssi = rssi;
  msg->nodeType = nodeType;
  msg->type = type;
  msg->data = clock_value;

  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message);

  //sending
  NETSTACK_NETWORK.output(&addr);

  free(msg);
}

void create_unicast_message_data(linkaddr_t dest,linkaddr_t addr, int type, int data) {
  // Allocate memory for the message
  struct message_data *msg;
  msg = (struct message_data*) malloc(sizeof(struct message_data));
  //creation
  msg->addr = addr;
  msg->type = type;
  msg->data = data;
  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message_data);

  //sending
  NETSTACK_NETWORK.output(&dest);

  free(msg);
}


void create_unicast_clock_update(linkaddr_t coordinator, clock_time_t clock_value,clock_time_t time_slot_start,int window,int num_coordinator){
    // Allocate memory for the message
    struct message_clock_update *msg;
    msg = (struct message_clock_update *)malloc(sizeof(struct message_clock_update));

    //creation
    msg->clock_value = clock_value;
    msg->time_slot_start = time_slot_start;
    msg->num_coordinator = num_coordinator;
    msg->window = window;
    msg->type = 9;
    // Set nullnet buffer and length
    nullnet_buf = (uint8_t *)msg;
    nullnet_len = sizeof(struct message_clock_update);

    //sending
    NETSTACK_NETWORK.output(&coordinator);

    free(msg);
}
void create_multicast_clock_update(clock_time_t clock_value,int window,int num_coordinators){
    // Allocate memory for the message
    struct message_clock_update *msg;
    msg = (struct message_clock_update *)malloc(sizeof(struct message_clock_update));

    //creation
    msg->clock_value = clock_value;
    msg->num_coordinator = num_coordinators;
    msg->window = window;
    msg->type = 9;
    // Set nullnet buffer and length
    nullnet_buf = (uint8_t *)msg;
    nullnet_len = sizeof(struct message_clock_update);

    //sending
    NETSTACK_NETWORK.output(NULL);

    free(msg);
}