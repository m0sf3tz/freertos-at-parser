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
#include "state_core.h"

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static const char        TAG[] = "UART_CORE";
static const char AT_PORT[] = "/dev/ttyUSB0";
static int atfd;

typedef struct {                                     
     uint16_t len;
     uint8_t  buf[1024];
} new_line_t;

static new_line_t new_line;
extern QueueSetHandle_t line_feed_q;
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
      ESP_LOGI(TAG, "Read something, len = %d: %d", new_line.len, new_line.buf[0]);
      BaseType_t xStatus = xQueueSendToBack(line_feed_q, &new_line, 0);
    }
  }
}

void spawn_uart_thread(){
  atfd = open( "/dev/ttyUSB0", O_RDWR);
  if (atfd == -1){
     ESP_LOGE(TAG, "Failed to open UART PORT!"); 
     ASSERT(0);
  }

  xTaskCreate(at_parser, "AT_PARSER", 2048, NULL, 5, NULL); 
}
