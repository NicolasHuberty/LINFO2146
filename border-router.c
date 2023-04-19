#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "dev/serial-line.h"
#include "dev/leds.h"
#include <string.h>
#include <stdio.h> 
#include "sys/log.h"
#define LOG_MODULE "Border Router"
#define LOG_LEVEL LOG_LEVEL_INFO

#define BRIDGE_PORT 1234
typedef struct message_struct{
    char[2] message_type;
    ///...TO CONTINUE
    char* message;
}message;

typedef struct node_structure{
    char[1] type; // 0 => Sensor, 1 => Coordinator
    int addr;
}node;

//TO change
static linkaddr_t sensor_node_addr = {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

PROCESS(border_router_process, "Border Router Process");
AUTOSTART_PROCESSES(&border_router_process);

void input_callback(const void *data, uint16_t len,const linkaddr_t *src, const linkaddr_t *dest){
  if(len == sizeof(int)) {
    int received_value;
    memcpy(&received_value, data, sizeof(received_value));
    LOG_INFO("Received value %d from ", received_value);
    LOG_INFO_LLADDR(src);
    LOG_INFO_("\n");
    message *msg = malloc(sizeof(message));
    memcpy(&msg, &received_value, sizeof(received_value));
    // Forward the received data to the server
    printf("Forwarding data to server: %d\n", received_value);
    LOG_INFO("Message Type received: %s\n",msg->message_type);
    // Forward the data via the serial-line to the server
    serial_line_write_message(&received_value, sizeof(received_value));
    free(message);
  }
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize NullNet */
  nullnet_set_input_callback(input_callback);

  /* Set up the bridge between the node and the server */
  serial_line_init();
  serial_line_set_input_callback(input_callback);
  serial_line_set_baud_rate(BRIDGE_PORT);

  LOG_INFO("Border router started\n");

  PROCESS_END();
}
