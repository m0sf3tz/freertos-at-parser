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
void create_cgact_cmd(char * str, int size){
  if(!str){
    ESP_LOGI(TAG, "NULL str!");
    ASSERT(0);
  }

  memset(str, 0, size);
  sprintf(str, "AT+CGACT=1\r\n");
}

void create_cfun_en_cmd(char * str, int size){
  if(!str){
    ESP_LOGI(TAG, "NULL str!");
    ASSERT(0);
  }

  memset(str, 0, size);
  sprintf(str, "AT+CFUN=1\r\n");
}

void create_kcnxcfg_cmd(char * str, int size){
  if(!str){
    ESP_LOGI(TAG, "NULL str!");
    ASSERT(0);
  }

  memset(str, 0, size);
  sprintf(str, "AT+KCNXCFG=1,\"GPRS\",\"%s\"\r\n", NET_APN);
}

void create_kudpsend_cmd(char * str, int size, char * ip, uint16_t port, size_t len){
  if(!str){
    ESP_LOGI(TAG, "NULL str!");
    ASSERT(0);
  }

  memset(str, 0, size);
  sprintf(str, "AT+KUDPSND=1,\"%s\",%d,%ld\r\n", ip, port, len);
}

void create_kudprcv_cmd(char * str, int size, size_t bytes){
  if(!str){
    ESP_LOGI(TAG, "NULL str!");
    ASSERT(0);
  }

  if(bytes > size){
    ESP_LOGE(TAG, "bytes > buffer size!");
    ASSERT(0);
  }

  memset(str, 0, size);
  sprintf(str, "AT+KUDPRCV=1,%lu\r\n", bytes);
}
