#pragma once
#include "state_core.h"

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
void state_post_event(state_event_t event);
void cellular_state_spawner();
void set_cellular_state(bool state);
bool get_cellular_state();

/*********************************************************
*                                                GLOBALS *
*********************************************************/
extern QueueHandle_t outgoing_events_cellular_q;

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                                  ENUMS *
*********************************************************/
typedef enum {
    cellular_waiting_wifi = 0,
    cellular_waiting_prov,
    cellular_running,

    cellular_state_len //LEAVE AS LAST!
} cellular_state_e;


typedef enum {
    EVENT_A = 100,
    EVENT_B,

    wifi_event_len //LEAVE AS LAST!
} cellular_event_e;

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define MAX_QUEUE_DEPTH     (16)
#define INVALID_EVENT       (0xFFFFFFFF)
#define NET_SATE_MUTEX_WAIT (2500 / portTICK_PERIOD_MS)
#define NET_SATE_QUEUE_TO   (2500 / portTICK_PERIOD_MS)
