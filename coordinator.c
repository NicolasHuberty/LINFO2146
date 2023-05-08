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
int duration;
int window;
struct sensor_info sensors_info[256];
int nb_sensors = 0;
/*---------------------------------------------------------------------------*/
PROCESS(coordinator_node_process, "Coordinator");
AUTOSTART_PROCESSES(&coordinator_node_process);

// declaration of functions
void rcv_hello_msg_sensors(struct message *hello_msg);
void rcv_time_slot();

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

    /*Simulate a slot time assignement*/
    if (msg->type == 100 && msg->nodeType == BORDER_ROUTER)
    { // msg type should be the type who say the border rooter gives a time slot to the coordinator
      printf("Authorization to get data from the sensors nodes\n");
      rcv_time_slot();
    }
  }

  if (len == sizeof(struct message_data))
  {

    // Cast the message payload to a struct message_data pointer
    struct message_data *msg = (struct message_data *)data;

    if (msg->type == DATA)
    {

      sensors_info[nb_sensors].data = msg->data;
      printf("Coordinator received %d from sensor \n", sensors_info[nb_sensors].data);
      printf("There is currently %d sensors\n", nb_sensors);
    }
  }
  if (len == sizeof(struct message_clock_update))
  {
    printf("--------------------Receive an update message----------------------------- len of clock: %d and message: %d\n", (int)sizeof(struct message_clock_update), (int)sizeof(struct message));
    struct message_clock_update *msg = (struct message_clock_update *)data;
    set_custom_clock_offset(clock_time() - msg->clock_value);
    time_slot_start = custom_clock_time() + msg->time_slot_start;
    duration = msg->duration;
    window = msg->window;
    printf("Actual coord %d: %d :clockTime = %d,  New custom clock time = %d, new time_slot_start = %d,new duration =%d, new window = %d\n",(int)num_coordinator, (int)clock_time(), (int)(clock_time() - msg->clock_value), (int)custom_clock_time(), (int)time_slot_start, (int)duration,(int) window);
  }
}

/*---------------------------------------------------------------------------*/

void rcv_time_slot()
{

  // Loop over the sensors_info table
  for (int i = 0; i < nb_sensors; i++)
  {
    // Access the current sensor_info element using the index i
    if (!linkaddr_cmp(&sensors_info[i].addr, &linkaddr_null))
    { // if the addr of the i element is not null
      struct sensor_info current_sensor = sensors_info[i];
      create_unicast_message(current_sensor.addr, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, ALLOW_SEND_DATA, 0);
    }
  }
  printf("Finished recolting data from all the sensors \n");
  if (nb_sensors > 0)
  {
    // create_multicast_transfer_data(100,100);
    // create_multicast_message(0,0,0,0);
    // create_unicast_message(border_router,10,10,10,10);
    create_unicast_transfer_data(border_router, TRANSFER_DATA, sensors_info, nb_sensors);
    printf("Sending data to the border_router with addr %d %d\n", border_router.u8[0], border_router.u8[1]);
    printf("sensor addr val is : %d%d\n", sensors_info[0].addr.u8[0], sensors_info[0].addr.u8[1]);
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
  while (1)
  {
    PROCESS_WAIT_EVENT();
    // PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    // printf("My custom clock time = %d, my time_slot_start = %d, the duration of the time slot = %d\n", (int)custom_clock_time(), (int)time_slot_start, duration);
    while (custom_clock_time() > time_slot_start && custom_clock_time() < time_slot_start + duration)
    {
      printf("Entering in my assigned time slot\n");
      time_slot_start += window;
      PROCESS_WAIT_EVENT();
      // PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      // send_data();
    }

    // Send the message
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}