#pragma once
#include "state_core.h"

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
void state_post_event(state_event_t event);
void net_state_spawner();
void set_net_state(bool state);
bool get_net_state();

/*********************************************************
*                                                GLOBALS *
*********************************************************/
extern QueueHandle_t outgoing_events_net_q;

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                                  ENUMS *
*********************************************************/
typedef enum {
    net_waiting_wifi = 0,
    net_waiting_prov,
    net_running,

    net_state_len //LEAVE AS LAST!
} net_state_e;


typedef enum {
    EVENT_A = 100,
    EVENT_B,

    wifi_event_len //LEAVE AS LAST!
} net_event_e;

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define MAX_QUEUE_DEPTH     (16)
#define INVALID_EVENT       (0xFFFFFFFF)
#define NET_SATE_MUTEX_WAIT (2500 / portTICK_PERIOD_MS)
#define NET_SATE_QUEUE_TO   (2500 / portTICK_PERIOD_MS)
