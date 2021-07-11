#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"
#include "event_groups.h"

/*********************************************************
*                                                DEFINES *
*********************************************************/
#define MAX_PACKET_LEN (256)
#define MAX_UDP_Q_LEN  (2)

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef struct{ 
  uint16_t len; // len can be 256 - won't fit in uint8.... 
  uint8_t  data[MAX_PACKET_LEN]; 
}udp_packet_s; 

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
void net_adaptor_spawner();
void enqueue_udp_write(udp_packet_s * udp);
void net_adaptor_init_freertos_objects();

/*********************************************************
*                                                GLOBALS *   
*********************************************************/
extern QueueSetHandle_t incoming_udp_q; 
extern QueueSetHandle_t outgoing_udp_q; 
