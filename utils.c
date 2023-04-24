#include "utils.h"
#include <stdlib.h>
#include "net/nullnet/nullnet.h"
#include "net/linkaddr.h"


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

void create_unicast_message_data(linkaddr_t addr, int type, int random_value) {
  // Allocate memory for the message
  struct message_data *msg;
  msg = (struct message_data*) malloc(sizeof(struct message_data));

  //creation
  msg->addr = addr;
  msg->type = type;
  msg->data = random_value;

  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message_data);

  //sending
  NETSTACK_NETWORK.output(&addr);

  free(msg);
}

void create_unicast_transfer_data(linkaddr_t border_router_addr, int type,struct sensor_info sensors[256], int nb_sensors){
  // Allocate memory for the message
  struct message_array_data *msg;
  msg = (struct message_array_data*) malloc(sizeof(struct message_array_data));

  //creation
  msg->addr = border_router_addr;
  msg->type = type;
  msg->nb_sensors = nb_sensors;
  for (int i = 0; i < nb_sensors; i++)
  {
    msg->sensors[i] = sensors[i];
  }
  
  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message_array_data);

  //sending
  NETSTACK_NETWORK.output(&border_router_addr);

  free(msg);

}