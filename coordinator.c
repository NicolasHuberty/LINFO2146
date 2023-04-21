#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 

#define HELLO_TYPE 1
#define ASK_CLOCK_TYPE 2
#define GIVE_CLOCK_TYPE 3
#define SET_CLOCK_TYPE 4 //rssi used to store the time slot
/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)
linkaddr_t border_router;
struct message {
 int rssi;
 int nodeType;
 int type;
 int data;
};

/*---------------------------------------------------------------------------*/
PROCESS(coordinator_node_process, "Coordinator");
AUTOSTART_PROCESSES(&coordinator_node_process);


void rcv_hello_msg(struct message *hello_msg);//declaration of the function

/*---------------------------------------------------------------------------*/
void create_send_message(int rssi, int eachNodeType,int type, int data){
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //creation
  msg->rssi = rssi;
  msg->nodeType = eachNodeType;
  msg->type = type;
  msg->data = data;

  // Set nullnet buffer and length
  nullnet_buf = (uint8_t *)msg;
  nullnet_len = sizeof(struct message);

  //sending
  NETSTACK_NETWORK.output(NULL);

  free(msg);
}

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{

  // Check if the message length is correct
  if(len != sizeof(struct message)) {
    printf("Invalid message length: %d (expected %d)\n", (int)len,(int) sizeof(struct message));
    return;
  }
  // Cast the message payload to a struct message pointer
  struct message *msg = (struct message*) data;

  // Print the message fields
  printf("Received message from node %d.%d:\n", src->u8[0], src->u8[1]);
  printf("RSSI: %d\n", msg->rssi);
  printf("eachNodeType: %d\n", msg->nodeType);
  printf("Type: %d\n",msg->type);
  printf("data: %d\n", msg->data);
  if(msg->type == HELLO_TYPE && msg->nodeType == 3){
    linkaddr_copy(&border_router, src);
    printf("Have receive the address of the border router\n");
  }
  /*Send answer to Hello message*/
  if(msg->type == HELLO_TYPE ) { //specific data for hello message
    //rcv_hello_msg(msg);
    printf("Hello msg received with type: %d\n",msg->nodeType);
  }
  printf("Before check\n");
  if(msg->type == ASK_CLOCK_TYPE){
    printf("Try to send clock_time from coordinator\n");
    create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI),2,GIVE_CLOCK_TYPE,clock_time());
  }


  
}
/*---------------------------------------------------------------------------*/

void rcv_hello_msg(struct message *hello_msg){

  create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 1,1, 100); //give specific data number so the sensor node know the coordinator is a parent candidate?
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coordinator_node_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  nullnet_set_input_callback(input_callback);
  create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI),2,1,42);
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    // Send the message
    //create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 2, 42);

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}