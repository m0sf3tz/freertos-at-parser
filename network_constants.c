#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include <errno.h>

#include "global_defines.h"

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                       NETWORK CONSTANTS *
**********************************************************/
static const char         TAG[]     = "NET_CONST";
static const char         NET_APN[] = "m2minternet.apn";

/**********************************************************
*                                               FUNCTIONS *
**********************************************************/
void create_kcnxcfg_cmd(char * str, int size){
  if(!str){
    ESP_LOGI(TAG, "NULL str!");
    ASSERT(0);
  }

  memset(str, 0, size);
  sprintf(str, "AT+KCNXCFG=1,\"GPRS\",\"%s\"\r\n", NET_APN);
  printf(str);
}
