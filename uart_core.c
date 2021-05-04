#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>

#include "global_defines.h"
#include "uart_core.h"
#include "at_parser.h"


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
static const char  AT_PORT[] = "/dev/ttyUSB1";
static int atfd;

static new_line_t new_line;
static QueueSetHandle_t command_issue_q;

extern QueueSetHandle_t line_feed_q; //todo move into proper place!
/**********************************************************
*                                               FUNCTIONS *
**********************************************************/


void at_parser(void *arg){
  for(;;){
    vTaskDelay(100/portTICK_PERIOD_MS);
    memset(&new_line, 0, sizeof(new_line_t));
    int count = read(atfd, new_line.buf, 1024);
    new_line.len = count;


    if(count > 0){
      BaseType_t xStatus = xQueueSendToBack(line_feed_q, &new_line, 0);
    }
  }
}

int at_command_issue_hal(char *cmd){
  if(!cmd){
    ESP_LOGE(TAG, "CMD == null!");
    ASSERT(0);
  }

#ifdef POSIX_FREERTOS_SIM     
  int rc = write(atfd, cmd, MAX_WRITE_COMMAND_LEN);
  if (rc == -1){
    ESP_LOGE(TAG, "Failed to write to UART HAL!");
    return UART_HAL_WRITE_ERROR; 
  }
  else{
    return UART_HAL_WRITE_OK;
  }
#endif
}

void init_cellular_freertos_objects (){
 command_issue_q = xQueueCreate(2, MAX_WRITE_COMMAND_LEN); //TODO: make constant
 
 ASSERT(command_issue_q);
}


void spawn_uart_thread(){
  atfd = open( AT_PORT, O_RDWR);
  if (atfd == -1){
     ESP_LOGE(TAG, "Failed to open UART PORT!"); 
     ASSERT(0);
  }

  xTaskCreate(at_parser, "AT_PARSER", 2048, NULL, 5, NULL); 
  //xTaskCreate(at_writter, "AT_WRITTER", 2048, NULL, 5, NULL); 
}
