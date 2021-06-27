#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include <errno.h>

#include "network_test.h"
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
static const char      TAG[] = "SIM_STREAM";

static char at_cfun_good_0[] = "AT+CFUN?\r\n";
static char at_cfun_good_1[] = "+CFUN: 1\r\n";
static char at_cfun_good_2[] = "+CFUN: 2\r\n";
static char at_cfun_good_3[] = "\r\n";
static char at_cfun_good_4[] = "OK\r\n";

static char at_cfun_error_0[] = "AT+CFUN=0\r\n";
static char at_cfun_error_1[] = "ERROR\r\n";

static char at_cfun_cme_0[] = "AT+CFUN=0\r\n";
static char at_cfun_cme_1[] = "+CME ERROR: 16\r\n";

static char at_cfun_urc_0[] = "+CEREG: 1\r\n";
static char at_cfun_urc_1[] = "AT+CFUN?\r\n";
static char at_cfun_urc_2[] = "+CFUN: 1\r\n";
static char at_cfun_urc_3[] = "\r\n";
static char at_cfun_urc_4[] = "OK\r\n";


static delay_command_s at_cfun_good = {
  .units[0] = {10, at_cfun_good_0},
  .units[1] = {20, at_cfun_good_1},
  .units[2] = {30, at_cfun_good_2},
  .units[3] = {40, at_cfun_good_3},
  .units[4] = {50, at_cfun_good_4},
};

static delay_command_s at_cfun_timeout = {
  .units[0] = {10, at_cfun_good_0},
  .units[1] = {20, at_cfun_good_1},
  .units[2] = {30, at_cfun_good_2},
  .units[3] = {6000, at_cfun_good_3},
  .units[4] = {50, at_cfun_good_4},
};

static delay_command_s at_cfun_error = {
  .units[0] = {10, at_cfun_error_0},
  .units[1] = {20, at_cfun_error_1},
};

static delay_command_s at_cfun_cme = {
  .units[0] = {10, at_cfun_cme_0},
  .units[1] = {20, at_cfun_cme_1},
};

static delay_command_s at_cfun_urc = {
  .units[0] = {10, at_cfun_urc_0},
  .units[1] = {20, at_cfun_urc_1},
  .units[2] = {30, at_cfun_urc_2},
  .units[3] = {40, at_cfun_urc_3},
  .units[4] = {50, at_cfun_urc_4}
};


/**********************************************************
*                                               FUNCTIONS *
**********************************************************/


static delay_command_s * curr_cmd;
static int curr_unit;
static int start_time;

void set_current_cmd(command_e cmd){
  curr_unit = 0;
  start_time = xTaskGetTickCount();

  switch(cmd){
    case(CFUN_GOOD):    curr_cmd = &at_cfun_good; return;
    case(CFUN_TIMEOUT): curr_cmd = &at_cfun_timeout; return;
    case(CFUN_ERROR):   curr_cmd = &at_cfun_error; return;
    case(CFUN_CME):     curr_cmd = &at_cfun_cme; return;
    case(CFUN_URC):     curr_cmd = &at_cfun_urc; return;
  }
}

#ifdef FAKE_INPUT_STREAM_MODE 


uint8_t* at_incomming_get_stream(int *len){
   int curr_time = xTaskGetTickCount() - start_time;
   curr_time = curr_time / portTICK_PERIOD_MS;  
  
   delay_command_s * tmp = curr_cmd;
   delay_unit_s      unit = curr_cmd->units[curr_unit];

   if(curr_time < unit.delay){
    return NULL;
   }

   curr_unit++;
   *len = strlen(unit.info);
   return (unit.info);
}


int at_command_issue_hal(char *cmd, int len){
}

#endif

static void * test_thread (void * arg){
  int len;
  set_current_cmd(CFUN);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  printf("%s \n", at_incomming_get_stream(&len));
}

void sim_stream_test(){
BaseType_t rc = xTaskCreate(test_thread,
                            "stream test",
                             4096,
                             NULL,
                             4,
                             NULL);


}
