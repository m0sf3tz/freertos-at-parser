#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include <errno.h>

#include "mailbox.h" 
#include "global_defines.h"
#include "network_state.h"
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
static const char         TAG[] = "NETWORK TEST";
static char               misc_buff[150];
/**********************************************************
*                                               FUNCTIONS *
**********************************************************/

static int verify_cfun_test(){
  return pdTRUE;
}

void network_test(){
  set_current_cmd(CFUN);
  create_cfun_en_cmd(misc_buff, 150);
  printf("%s \n", misc_buff);
  send_cmd(misc_buff, strlen(misc_buff), verify_cfun_test, CFUN);

  vTaskDelay(1000);
  set_current_cmd(CFUN);
  puts("sending second cmd1");
  send_cmd(misc_buff, strlen(misc_buff), verify_cfun_test, CFUN);

}
