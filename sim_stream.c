#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include <errno.h>

#include "sim_stream.h"
#include "global_defines.h"
#include "at_parser.h"

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static const char         TAG[] = "SIM_STREAM";

/**********************************************************
*                                               FUNCTIONS *
**********************************************************/


static char at_cfun_0[] = "AT+CFUN=1\r\n";
static char at_cfun_1[] = "OK\r\n";

static delay_command_s at_cfun = {
  .units[0] = {1000, at_cfun_0},
  .units[1] = {1000, at_cfun_1},
};

static delay_command_s * curr_cmd;
static int curr_unit;

void set_current_cmd(command_e cmd){
  curr_unit = 0;
  switch(cmd){
    case(CFUN):
      curr_cmd = &at_cfun;
      return;
  }
}

#ifdef FAKE_INPUT_STREAM_MODE 


uint8_t* at_incomming_get_stream(int *len){

   puts("READ!");
   delay_command_s * tmp = curr_cmd;
   delay_unit_s      unit = curr_cmd->units[curr_unit];

   curr_unit++;
   printf("%d %s", unit.delay, unit.info);
   *len = strlen(unit.info);
   return (unit.info);
}


int at_command_issue_hal(char *cmd, int len){

}

#endif
