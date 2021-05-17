#pragma once
#include "state_core.h"

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/

/*********************************************************
*                                                GLOBALS *
*********************************************************/

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef int net_mode_t;
typedef int cmd_t;
typedef int network_status_t;

typedef struct{
  net_mode_t       mode;
  cmd_t            curr_cmd;
  network_status_t net_state;  
}network_state_s;

typedef enum {
    network_attaching_state = 0,
    network_idle_state,

    network_state_len //LEAVE AS LAST!
} network_state_e;


typedef enum {
    NETWORK_ATTACHED = NETWORK_EVENT_START,
    NETWORK_DETACHED,

    network_event_len //LEAVE AS LAST!
} network_event_e;


/*********************************************************
*                                                  ENUMS *
*********************************************************/

/*********************************************************
*                                                 DEFINES *
*********************************************************/
