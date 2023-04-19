#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h> 

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)

struct message {
 int rssi;
 int eachNodeType;
 int data;
};

/*---------------------------------------------------------------------------*/
PROCESS(coordinator_node_process, "Coordinator");
AUTOSTART_PROCESSES(&coordinator_node_process);


void rcv_hello_msg(struct message *hello_msg);//declaration of the function

/*---------------------------------------------------------------------------*/
void create_send_message(int rssi, int eachNodeType, int data){
  // Allocate memory for the message
  struct message *msg;
  msg = (struct message*) malloc(sizeof(struct message));

  //creation
  msg->rssi=rssi;
  msg->eachNodeType=eachNodeType;
  msg->data=data;

 //sending
  uint8_t len = sizeof(struct message);
  void *payload = (void*)msg;
  memcpy(nullnet_buf, payload, len);
  nullnet_len = len;
  NETSTACK_NETWORK.output(NULL);

  free(msg);
}
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  // Check if the message length is correct
  if(len != sizeof(struct message)) {
    printf("Invalid message length: %d (expected %d)\n", len, sizeof(struct message));
    return;
  }
  // Cast the message payload to a struct message pointer
  struct message *msg = (struct message*) data;

  // Print the message fields
  printf("Received message from node %d.%d:\n", src->u8[0], src->u8[1]);
  printf("RSSI: %d\n", msg->rssi);
  printf("eachNodeType: %d\n", msg->eachNodeType);
  printf("data: %d\n", msg->data);

  /*Send answer to Hello message*/
  if(msg->data == 42) { //specific data for hello message
    rcv_hello_msg(msg);
    printf("Hello msg received\n");
  }
}
/*---------------------------------------------------------------------------*/

void rcv_hello_msg(struct message *hello_msg){

  create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 1, 100); //give specific data number so the sensor node know the coordinator is a parent candidate?
}


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coordinator_node_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  nullnet_set_input_callback(input_callback);

  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    printf("Sending");
    printf("\n");
    
    // Send the message
    create_send_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 1, 42);

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}