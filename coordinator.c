#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

#define SEND_INTERVAL (8 * CLOCK_SECOND)

static struct etimer periodic_timer;
static struct etimer sensors_slot;
static clock_time_t custom_clock_offset = 0;
clock_time_t time_slot_start;
linkaddr_t border_router;
int num_coordinator = 0;
int window; 
int num_total_coordinators = 0;
int nb_sensors = 0;
int current_sensor = -1;
int alive = 1; //variable that store if sensors are responsive
struct sensor_info sensors_info[256]; //array of sensors

/*Set an offset to modify the clock_time of the mote*/
void set_custom_clock_offset(clock_time_t offset){
  custom_clock_offset = offset;
}

/*Get the adapted clock_time*/
clock_time_t custom_clock_time(){
  return clock_time() - custom_clock_offset;
}

/*Check if the address is existing in the sensors */
int is_addr_present(linkaddr_t addr){
  for (int i = 0; i < nb_sensors; i++){
    if (linkaddr_cmp(&sensors_info[i].addr, &addr)){
      return 1;
    }
  }
  return 0;
}

/*Remove the sensor on the sensor list */
void delete_sensor(linkaddr_t sensor_addr){
    int sensor_index = -1;
    for (int i = 0; i < nb_sensors; i++){
      if (linkaddr_cmp(&sensors_info[i].addr, &sensor_addr)){
        sensor_index = i;
        if (sensor_index != -1){
          /*Shift all sensors in the list*/
          for (int j = i; j < nb_sensors - 1; j++){
            sensors_info[j] = sensors_info[j+1];
          }
        nb_sensors -= 1;
        //printf("One sensor deleted from list, there is now %d sensors \n", nb_sensors);
        break;
        }
      }
    }
} 
void rcv_hello_msg_sensors(struct message *hello_msg);

/*---------------------------------------------------------------------------*/
PROCESS(coordinator_node_process, "Coordinator");
AUTOSTART_PROCESSES(&coordinator_node_process);

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
                    const linkaddr_t *src, const linkaddr_t *dest){
  /*Receive a network message packet*/
  if (len == sizeof(struct message)){
    struct message *msg = (struct message *)data;
    /*Discover the border router*/
    if (msg->type == HELLO_TYPE && msg->nodeType == BORDER_ROUTER){
      linkaddr_copy(&border_router, src);
      printf("Discover the border router addr: %d%d\n", src->u8[0], src->u8[1]);
      create_unicast_message(border_router, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, HELLO_TYPE, clock_time());
      num_coordinator = (int)msg->data; //Update identifier of the coordinator
    }
    /*Discover a sensor*/
    if (msg->type == HELLO_TYPE && msg->nodeType == SENSOR){
      create_unicast_message(*src, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, RESPONSE_HELLO_MSG, 0);
    }

    /*Add sensor to list of children*/
    if (msg->type == CHOSEN_PARENT && msg->nodeType == SENSOR && is_addr_present(*src) == 0){
      struct sensor_info new_sensor;
      new_sensor.addr = *src;
      sensors_info[nb_sensors++] = new_sensor;
      printf("Sensor add to the list of sensors with addr: %d%d\n", new_sensor.addr.u8[0], new_sensor.addr.u8[1]);
    }

    /*Send clock*/
    if (msg->type == ASK_CLOCK_TYPE){
      create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, GIVE_CLOCK_TYPE, clock_time());
    }
  }
  /*Transfer a data from a sensor to the border router*/
  if (len == sizeof(struct message_data)){
    struct message_data *msg = (struct message_data *)data;
    alive = 1;
    if(msg->type == NOT_MY_DATA){
<<<<<<< Updated upstream
      create_unicast_message_data(border_router,*src,DATA,msg->data); //TO adapt taking the address
=======
      create_unicast_message_data(border_router,msg->addr,DATA,msg->data);
>>>>>>> Stashed changes
    }else{
      create_unicast_message_data(border_router,*src,DATA,msg->data);
    }
    //printf("Coordinator transfer %d from sensor \n",(int) msg->data);
  }
  /*Receive a message to synchronize the clock_time*/
  if (len == sizeof(struct message_clock_update)){
    struct message_clock_update *msg = (struct message_clock_update *)data;
    /*Adapt the offset to be synchronized in the network*/
    set_custom_clock_offset(clock_time() - msg->clock_value);
    num_total_coordinators = msg->num_coordinator;
    /*Set all necessary informations to calculate the time slot*/
    if(num_total_coordinators > 0){
      window = msg->window;
      time_slot_start = msg->clock_value + (num_coordinator* (window/num_total_coordinators));
      if(nb_sensors > 0){
          etimer_set(&sensors_slot,(window/num_total_coordinators)/nb_sensors);
      }
      //printf("Actual coord %d: %d :clockTime = %d,  New custom clock time = %d, new time_slot_start = %d, new window = %d\n",(int)num_coordinator, (int)clock_time(), (int)(clock_time() - msg->clock_value), (int)custom_clock_time(), (int)time_slot_start, (int) window);
      alive = 1;
    }else{
      /*Other way to discover the border router if coordinator mote spawn after border*/
      linkaddr_copy(&border_router, src);
      printf("----------------------------Have receive the address of the border router addr: %d%d\n", src->u8[0], src->u8[1]);
      create_unicast_message(border_router, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, HELLO_TYPE, clock_time());
    }
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coordinator_node_process, ev, data){
  PROCESS_BEGIN();
  create_multicast_message(packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, HELLO_TYPE, 0);
  nullnet_set_input_callback(input_callback);
  etimer_set(&periodic_timer, 1);
  etimer_set(&sensors_slot,10000000);
  while (1){
    PROCESS_WAIT_EVENT();
    /*Check if it is the allocated time slot*/
    if(num_total_coordinators > 0 && custom_clock_time() > time_slot_start && custom_clock_time() < time_slot_start + window && nb_sensors > 0){
      /*Launch timeslot of next sensor*/
      if(etimer_expired(&sensors_slot) || current_sensor == -1){

        /*Remove the sensor if needed*/
        if (alive == 0){ 
          if (current_sensor == -1){
            printf("last Sensor is not responding\n");
            create_unicast_message_data(border_router, sensors_info[nb_sensors-1].addr, DATA, -1);
            delete_sensor(sensors_info[nb_sensors-1].addr);
          }
          else{
            printf("Sensor is not responding\n");
            create_unicast_message_data(border_router, sensors_info[nb_sensors-1].addr, DATA, -1);
            delete_sensor(sensors_info[current_sensor].addr);
            current_sensor -= 1;    
          }
        }
        alive = 0;
        current_sensor+=1;
        create_unicast_message(sensors_info[current_sensor].addr, packetbuf_attr(PACKETBUF_ATTR_RSSI), COORDINATOR, ALLOW_SEND_DATA, 0);
        printf("Send allow data to sensor\n");
        etimer_reset(&sensors_slot);
        if(current_sensor == nb_sensors-1){
          current_sensor = -1;
          time_slot_start += window;
          etimer_reset(&sensors_slot);        
        }
      }
    }
    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}