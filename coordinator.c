#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

/* Configuration */
#define SEND_INTERVAL (8 * CLOCK_SECOND)
linkaddr_t border_router;

struct sensor_info sensors[256];
int nb_sensors = 0;
/*---------------------------------------------------------------------------*/
PROCESS(coordinator_node_process, "Coordinator");
AUTOSTART_PROCESSES(&coordinator_node_process);

//declaration of functions
void rcv_hello_msg_sensors(struct message *hello_msg);
void rcv_time_slot();

int is_addr_present(linkaddr_t addr) {
  for (int i = 0; i < nb_sensors; i++) {
    if (linkaddr_cmp(&sensors[i].addr, &addr)) {
      return 1; // Address is present in the array
    }
  }
  return 0; // Address is not present in the array
}
/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{

  // Check if the message length is correct
  if(len != sizeof(struct message) && len != sizeof(struct message_data)) {
    printf("Invalid message length: %d (expected %d)\n", (int)len,(int) sizeof(struct message));
    return;
  }
  if (len == sizeof(struct message)){
      // Cast the message payload to a struct message pointer
      struct message *msg = (struct message*) data;

      // Print the message fields
      /*
      printf("Received message from node %d.%d:\n", src->u8[0], src->u8[1]);
      printf("RSSI: %d\n", msg->rssi);
      printf("eachNodeType: %d\n", msg->nodeType);
      printf("Type: %d\n",msg->type);
      printf("data: %d\n", msg->data);
      */


      /*Get the addr of the border rooter*/
      if(msg->type == HELLO_TYPE && msg->nodeType == 3){
        linkaddr_copy(&border_router, src);
        printf("Have receive the address of the border router\n");
      }

      /*Send answer to Hello message*/
      if(msg->type == HELLO_TYPE && msg->nodeType == 0) {
        printf("Hello msg received with type: %d\n",msg->nodeType);
        create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), 2, RESPONSE_HELLO_MSG, 0);
      }

      /*Add sensor to list of children*/
      if (msg->type == CHOSEN_PARENT && msg->nodeType == 0 && is_addr_present(*src) == 0){
        printf("Sensor choose coordinator as parent\n");
        struct sensor_info new_sensor;
        new_sensor.addr = *src;
        new_sensor.data = 0;
        sensors[nb_sensors] = new_sensor;
        nb_sensors += 1;
        printf("Sensor add to the list of sensors\n");
      }

      /*Send clock*/
      if(msg->type == ASK_CLOCK_TYPE){
        printf("Try to send clock_time from coordinator\n");
        create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI),2,GIVE_CLOCK_TYPE,clock_time());
      }

      /*Simulate a slot time assignement*/
      if(msg->type == 100 && msg->nodeType == 3){ //msg type should be the type who say the border rooter gives a time slot to the coordinator
        printf("Authorization to get data from the sensors nodes\n");
        rcv_time_slot();
      }

  }

  if (len == sizeof(struct message_data)){

    // Cast the message payload to a struct message_data pointer
    struct message_data *msg = (struct message_data*) data;

    if(msg->type == DATA){
      printf("I received data from a sensor\n");
      sensors[nb_sensors].data = msg->data;
    }
  }

}


/*---------------------------------------------------------------------------*/

void rcv_time_slot(){

  // Loop over the sensors_info table
  for (int i = 0; i < nb_sensors; i++) {
    // Access the current sensor_info element using the index i
    if (!linkaddr_cmp(&sensors[i].addr, &linkaddr_null)){ //if the addr of the i element is not null
      struct sensor_info current_sensor = sensors[i];
      create_unicast_message(current_sensor.addr, packetbuf_attr(PACKETBUF_ATTR_RSSI),1, ALLOW_SEND_DATA, 0); 
    }
  }
  if (nb_sensors > 0){
    create_unicast_transfer_data(border_router, TRANSFER_DATA, sensors, nb_sensors);
    printf("Sending data to the border_router\n");
  }

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coordinator_node_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), 1, HELLO_TYPE, 0); //send hello message to find the border rooter
  
  nullnet_set_input_callback(input_callback);
  etimer_set(&periodic_timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    // Send the message

    
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}