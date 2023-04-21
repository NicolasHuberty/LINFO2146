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

struct message {
  int rssi;
  int nodeType;
  int type;
  int data;
};
void create_multicast_message(int rssi, int eachNodeType,int type,clock_time_t clock_value);
void create_unicast_message(linkaddr_t addr,int rssi, int eachNodeType,int type,clock_time_t clock_value);
#endif // UTILS_H_
