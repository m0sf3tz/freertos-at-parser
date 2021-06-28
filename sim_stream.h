#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"
#include "event_groups.h"
#include "at_parser.h"
#include "state_core.h"
#include "global_defines.h"

/*********************************************************
*                                                DEFINES *
*********************************************************/
#define MAX_DELAY_UNITS (50)
#define DELAY_UNIT      (10)

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

// simulation state machine
typedef enum {
    sim_idle_state = 0,
    sim_handle_cmd_state,
    sim_handle_write_state,

    sim_state_len //LEAVE AS LAST!
} parser_state_e;

typedef enum {
    EVENT_SIMULATE_CMD = SIM_EVENT_START,
    EVENT_SIMULATE_WRITE,

    sim_event_len //LEAVE AS LAST!
} sim_event_e;

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
#ifdef FAKE_INPUT_STREAM_MODE
uint8_t * at_incomming_get_stream(int *len);
int       at_command_issue_hal(char *cmd, int len);
#endif

void reset_sim_state_machine();
void set_current_cmd(command_e cmd);
void sim_sream_test();
void start_sim_write();
/*********************************************************
*                                                GLOBALS *   
*********************************************************/
