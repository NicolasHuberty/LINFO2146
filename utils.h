// utils.h

#ifndef UTILS_H_
#define UTILS_H_

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 
#include "utils.h"

/*Define all type of message*/
#define HELLO_TYPE 1
#define ASK_CLOCK_TYPE 2
#define GIVE_CLOCK_TYPE 3
#define SET_CLOCK_TYPE 4 //rssi used to store the time slot
#define ALLOW_SEND_DATA 5 // coordinator allow sensor node to send his data
#define DATA 6 // data message 
#define CHOSEN_PARENT 7 //sensor choose node as parent
#define RESPONSE_HELLO_MSG 8 // node respond to a hello_msg
#define TRANSFER_DATA 9 // coordinator transfering data to border router

#define BORDER_ROUTER 100 //2
#define COORDINATOR 101 //1
#define SENSOR 102 //0

struct message {
  int rssi;
  int nodeType;
  int type;
  int data;
};

struct message_data {
  linkaddr_t addr;
  int type;
  int data;
};


struct sensor_info {
  linkaddr_t addr;
  int data;

};

struct message_array_data {
  linkaddr_t addr;
  int type;
  struct sensor_info sensors[256];
  int nb_sensors;
};

void create_multicast_message(int rssi, int eachNodeType,int type,clock_time_t clock_value);
void create_unicast_message(linkaddr_t addr,int rssi, int eachNodeType,int type,clock_time_t clock_value);
void create_unicast_message_data(linkaddr_t addr, int type, int data); //usually for sensors sending their data to coordinator
void create_unicast_transfer_data(linkaddr_t border_router_addr, int type, struct sensor_info sensors[256], int nb_sensors); // usually coordinator transfering data of the sensors to the border_router
#endif // UTILS_H_