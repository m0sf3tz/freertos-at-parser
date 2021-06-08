#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include <errno.h>

#include "global_defines.h"
#include "net_adaptor.h"
#include "network_state.h"

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/
QueueSetHandle_t incoming_udp_q;
QueueSetHandle_t outgoing_udp_q;

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static udp_packet_s udp_packet;
static const char        TAG[] = "NET_CORE";

/**********************************************************
*                                               FUNCTIONS *
**********************************************************/

static void process_reads(void * arg){
  for(;;){
    memset(&udp_packet, 0 , sizeof(udp_packet_s));
    xQueueReceive(incoming_udp_q , &udp_packet, portMAX_DELAY);

    for (int i = 0; i < 16; i++)
      printf (" %c ", udp_packet.data[i] );
    puts("");
    for (int i = 16; i < 32; i++)
      printf (" %c ", udp_packet.data[i] );
    puts("");
  } 
}

static void send_write(udp_packet_s * udp){
    BaseType_t xStatus = xQueueSendToBack(outgoing_udp_q, udp, RTOS_DONT_WAIT);
    if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send on event queue %s ", name);
        ASSERT(0);
    }
    state_post_event(NETWORK_WRITE_REQUEST); 
}


static void net_adaptor_init_freertos_objects() {
    incoming_udp_q = xQueueCreate(MAX_UDP_Q_LEN, sizeof(udp_packet_s));
    outgoing_udp_q = xQueueCreate(MAX_UDP_Q_LEN, sizeof(udp_packet_s));

    ASSERT(incoming_udp_q);
    ASSERT(outgoing_udp_q);
}

void net_adaptor_spawner() {
    BaseType_t rc;

    net_adaptor_init_freertos_objects();

    rc = xTaskCreate(process_reads,
                     "reader core",
                     2048,
                     NULL,
                     4,
                     NULL);

    if (rc != pdPASS) {
        ASSERT(0);
    }
}
