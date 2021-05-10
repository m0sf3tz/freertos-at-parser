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

/*********************************************************
*                                                  ENUMS *
*********************************************************/

/*********************************************************
*                                                 DEFINES *
*********************************************************/