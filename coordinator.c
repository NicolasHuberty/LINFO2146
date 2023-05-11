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
static clock_time_t custom_clock_offset = 0;
static struct etimer sensors_slot;

void set_custom_clock_offset(clock_time_t offset)
{
  custom_clock_offset = offset;
}
clock_time_t custom_clock_time()
{
  return clock_time() - custom_clock_offset;
}

linkaddr_t border_router;
int num_coordinator = 0;
clock_time_t time_slot_start;
int window;
int num_total_coordinators = 0;
struct sensor_info sensors_info[256];
int nb_sensors = 0;
int current_sensor = -1;
/*---------------------------------------------------------------------------*/
PROCESS(coordinator_node_process, "Coordinator");
AUTOSTART_PROCESSES(&coordinator_node_process);

// declaration of functions
void rcv_hello_msg_sensors(struct message *hello_msg);

int is_addr_present(linkaddr_t addr)
{
  for (int i = 0; i < nb_sensors; i++)
  {
    if (linkaddr_cmp(&sensors_info[i].addr, &addr))
    {
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
  if (len != sizeof(struct message) && len != sizeof(struct message_data) && len != sizeof(struct message_clock_update))
  {
    printf("Invalid message length: %d (expected %d) sizeof clock_message: %d\n", (int)len, (int)sizeof(struct message), (int)sizeof(struct message_clock_update));
    return;
  }
  if (len == sizeof(struct message))
  {
    // Cast the message payload to a struct message pointer
    struct message *msg = (struct message *)data;

    /*Get the addr of the border rooter*/
    if (msg->type == HELLO_TYPE && msg->nodeType == BORDER_ROUTER)
    {
      linkaddr_copy(&border_router, src);
      printf("----------------------------Have receive the address of the border router addr: %d%d\n", src->u8[0], src->u8[1]);
      create_unicast_message(border_router, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, HELLO_TYPE, clock_time());
      num_coordinator = (int)msg->data;
    }

    /*Send answer to Hello message*/
    if (msg->type == HELLO_TYPE && msg->nodeType == SENSOR)
    {
      printf("Hello msg received with type: %d\n", msg->nodeType);
      create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, RESPONSE_HELLO_MSG, 0);
    }

    /*Add sensor to list of children*/
    if (msg->type == CHOSEN_PARENT && msg->nodeType == SENSOR && is_addr_present(*src) == 0)
    {
      printf("Sensor choose coordinator as parent\n");
      struct sensor_info new_sensor;
      new_sensor.addr = *src;
      new_sensor.data = 0;
      sensors_info[nb_sensors] = new_sensor;
      nb_sensors += 1;
      printf("Sensor add to the list of sensors with addr: %d%d\n", new_sensor.addr.u8[0], new_sensor.addr.u8[1]);
    }

    /*Send clock*/
    if (msg->type == ASK_CLOCK_TYPE)
    {
      printf("Try to send clock_time from coordinator\n");
      create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, GIVE_CLOCK_TYPE, clock_time());
    }

  }

  if (len == sizeof(struct message_data))
  {

    // Cast the message payload to a struct message_data pointer
    struct message_data *msg = (struct message_data *)data;
    sensors_info[nb_sensors].data = (int)msg->data;
    create_unicast_message_data(border_router,*src,DATA,msg->data);
    printf("Coordinator transfer %d from sensor \n",(int) msg->data);
    }
  
  if (len == sizeof(struct message_clock_update))
  {
    printf("--------------------Receive an update message----------------------------- len of clock: %d and message: %d\n", (int)sizeof(struct message_clock_update), (int)sizeof(struct message));
    struct message_clock_update *msg = (struct message_clock_update *)data;
    set_custom_clock_offset(clock_time() - msg->clock_value);
    num_total_coordinators = msg->num_coordinator;
    if(num_total_coordinators > 0){
      window = msg->window;
      time_slot_start = msg->clock_value + (num_coordinator* (window/num_total_coordinators));
      printf("Nb sensors: %d",nb_sensors);
      if(nb_sensors > 0){
          printf("Set sensor slot at: %d\n",nb_sensors);
          etimer_set(&sensors_slot,(window/num_total_coordinators)/nb_sensors);
      }
      printf("Actual coord %d: %d :clockTime = %d,  New custom clock time = %d, new time_slot_start = %d, new window = %d\n",(int)num_coordinator, (int)clock_time(), (int)(clock_time() - msg->clock_value), (int)custom_clock_time(), (int)time_slot_start, (int) window);
    }else{
      linkaddr_copy(&border_router, src);
      printf("----------------------------Have receive the address of the border router addr: %d%d\n", src->u8[0], src->u8[1]);
      create_unicast_message(border_router, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, HELLO_TYPE, clock_time());
    }
   
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coordinator_node_process, ev, data)
{
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, HELLO_TYPE, 0); // send hello message to find the border rooter

  nullnet_set_input_callback(input_callback);
  etimer_set(&periodic_timer, 1);
  etimer_set(&sensors_slot,10000000);
  while (1)
  {
    PROCESS_WAIT_EVENT();
    if(num_total_coordinators > 0 && custom_clock_time() > time_slot_start && custom_clock_time() < time_slot_start + window && nb_sensors > 0)
    {
      if(etimer_expired(&sensors_slot) || current_sensor == -1){ //Sensors_slot = 250ms
        etimer_reset(&sensors_slot);
        current_sensor+=1;
        create_unicast_message(sensors_info[current_sensor].addr, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, ALLOW_SEND_DATA, 0);
        
        if(current_sensor == nb_sensors-1){
          current_sensor = -1;
          time_slot_start += window;
          etimer_set(&sensors_slot,window);
        }
      }
    }

    // Send the message
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}
