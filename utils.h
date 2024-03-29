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
#define REMOVE_PARENT 10
#define NOT_MY_DATA 11
#define DO_YOU_NEED_PARENT 12
#define BORDER_ROUTER 100 //2
#define COORDINATOR 101 //1
#define SENSOR 102 //0

struct sensor_info {
  linkaddr_t addr;
  int data;
};

struct coordinator_info {
  linkaddr_t addr;
  clock_time_t clock_value;
  clock_time_t time_slot_start;
  int nb_sensors;
  struct sensor_info sensors[256];
};

struct message {
  int rssi;
  int nodeType;
  int type;
  clock_time_t data;
};

struct message_data {
  linkaddr_t addr;
  int type;
  int data;
};

struct message_clock_update{
  int type;
  clock_time_t clock_value;  //3000
  clock_time_t time_slot_start; // 3600 -> 4600 -> 5600 -> 6600
  int window; //1000
  int num_coordinator; //200ms
};
void create_multicast_message(int rssi, int nodeType,int type,clock_time_t clock_value);
void create_unicast_message(linkaddr_t addr,int rssi, int nodeType,int type,clock_time_t clock_value);
void create_unicast_message_data(linkaddr_t dest,linkaddr_t addr, int type, int data); //usually for sensors sending their data to coordinator
void create_unicast_clock_update(linkaddr_t coordinator, clock_time_t clock_value,clock_time_t time_slot_start,int window,int num_coordinator);
void create_multicast_clock_update(clock_time_t clock_value,int window,int num_coordinators);

#endif // UTILS_H_