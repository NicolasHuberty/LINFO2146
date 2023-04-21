#include "utils.h"
#include <stdlib.h>
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"
#define HELLO_TYPE 1
#define ASK_CLOCK_TYPE 2
#define GIVE_CLOCK_TYPE 3
#define SET_CLOCK_TYPE 4 //rssi used to store the time slot

void create_multicast_message(int rssi, int eachNodeType,int type,clock_time_t clock_value) {
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //creation
  msg->rssi = rssi;
  msg->nodeType = eachNodeType;
  msg->type = type;
  msg->data = clock_value;
  //msg->clock_value = clock_value;
  //msg->time_slot = time_slot; // Add time slot field

  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message);

  //sending
  NETSTACK_NETWORK.output(NULL);

  free(msg);
}
void create_unicast_message(linkaddr_t addr,int rssi, int eachNodeType,int type,clock_time_t clock_value) {
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //creation
  msg->rssi = rssi;
  msg->nodeType = eachNodeType;
  msg->type = type;
  msg->data = clock_value;
  //msg->clock_value = clock_value;
  //msg->time_slot = time_slot; // Add time slot field

  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message);

  //sending
  NETSTACK_NETWORK.output(&addr);

  free(msg);
}