#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"
#include "event_groups.h"
#include "at_parser.h"

/*********************************************************
*                                                DEFINES *
*********************************************************/
#define MAX_DELAY_UNITS (50)

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef struct{
  uint32_t delay;
  char * info;
}delay_unit_s;

typedef struct{
  delay_unit_s units[MAX_DELAY_UNITS];
}delay_command_s;


/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/

uint8_t * at_incomming_get_stream(int *len);
int       at_command_issue_hal(char *cmd, int len);

void      set_current_cmd(command_e cmd);

/*********************************************************
*                                                GLOBALS *   
*********************************************************/
