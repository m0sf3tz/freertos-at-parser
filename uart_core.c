#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>

#include "global_defines.h"
#include "uart_core.h"
#include "at_parser.h"

#ifdef POSIX_FREERTOS_SIM
 #include <sys/ioctl.h> 
#endif 

#define MAX_WRITE_COMMAND_LEN (150)

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static const char      TAG[] = "UART_CORE";
static const char  AT_PORT[] = "/dev/ttyUSB0";
static int atfd;
extern QueueSetHandle_t line_feed_q; //todo move into proper place!
static uint8_t buff_s[512];

/**********************************************************
*                                               FUNCTIONS *
**********************************************************/
// On POISX, this uses ioctl to see incomming bytes
// on ESP32, we hook into the UART interrupts
bool at_incomming_peek(){
#ifdef POSIX_FREERTOS_SIM     
  for(;;){
    
    int count = 0;        
    if ( ioctl(atfd, FIONREAD, &count) < 0 )  {
      ESP_LOGE(TAG, "Failed to issue ioctl!");
    }

    if(count > 0) return true;

    return false;
  }
#endif
}

#ifndef FAKE_INPUT_STREAM_MODE

// Returns a refernce to a bunffer,
// len == -1 on error, data read otherwise
uint8_t* at_incomming_get_stream(int *len){
  if (!len){
    ESP_LOGE(TAG, "Null parameter!");
    ASSERT(0);
  }

  int rc = read(atfd, buff_s, sizeof(buff_s));
  if ( rc > 0 ){
    *len = rc;
#ifdef POSIX_FREERTOS_SIM
    { // test. can remov
      char test[2000];
      memset(test, 0 , 200);
      memcpy(test, buff_s, rc);
      printf("ECHO (%d):> %s \n", rc, test);
      
      for(int i = 0; i < rc; i++){
        printf("%c (%d), \n", test[i], test[i]);
      }
    }
#endif 
    return buff_s;
  }
  return NULL;
}

int at_command_issue_hal(char *cmd, int len){
  if(!cmd){
    ESP_LOGE(TAG, "CMD == null!");
    ASSERT(0);
  }
  ESP_LOGI(TAG, "Sending command %s", cmd);

#ifdef POSIX_FREERTOS_SIM     
  int rc = write(atfd, cmd, len);
  if (rc == -1){
    ESP_LOGE(TAG, "Failed to write to UART HAL!");
    return UART_HAL_WRITE_ERROR; 
  }
  else{
    return UART_HAL_WRITE_OK;
  }
#endif
}

#endif 


void spawn_uart_thread(){
#ifndef FAKE_INPUT_STREAM_MODE
  atfd = open( AT_PORT, O_RDWR);
  if (atfd == -1){
     ESP_LOGE(TAG, "Failed to open UART PORT!"); 
     ASSERT(0);
  }

  // set to non-blocking 
  int flags = fcntl(atfd, F_GETFL, 0);
  fcntl(atfd, F_SETFL, flags | O_NONBLOCK);
#endif
}
