#pragma once
#include "state_core.h"

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
void state_post_event(state_event_t event);
void parser_state_spawner();
void set_parser_state(bool state);
bool get_parser_state();
uint8_t * at_parser_main(bool data_mode, bool * status, int * size);

/*********************************************************
*                                                GLOBALS *
*********************************************************/
extern QueueHandle_t outgoing_events_parser_q;

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                                  ENUMS *
*********************************************************/
typedef enum {
    parser_waiting_wifi = 0,
    parser_waiting_prov,
    parser_running,

    parser_state_len //LEAVE AS LAST!
} parser_state_e;


typedef enum {
    EVENT_NEW_UART_DATA_F = PARSER_CORE_EVENT_START,

    parser_event_len //LEAVE AS LAST!
} parser_event_e;

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define MAX_QUEUE_DEPTH     (16)
#define INVALID_EVENT       (0xFFFFFFFF)
#define NET_SATE_MUTEX_WAIT (2500 / portTICK_PERIOD_MS)
#define NET_SATE_QUEUE_TO   (2500 / portTICK_PERIOD_MS)
