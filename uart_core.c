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
      BaseType_t xStatus = xQueueSendToBack(line_feed_q, &new_line, 0);
    }
  }
}

void at_writter(void *arg){
  for(;;){

    int len; 
 
    vTaskDelay(2500/portTICK_PERIOD_MS);
    char ateoff[50] = "ATE0";
    len = strnlen(ateoff, 100);
    ateoff[len] = '\r';
    write(atfd, ateoff, len+1);

    vTaskDelay(2500/portTICK_PERIOD_MS);
    char command[50] = "AT+KCNXCFG=1,\"GPRS\",\"m2minternet.apn\"";
    len = strnlen(command, 100);
    command[len] = '\r';
    int rc = write(atfd, command, len+1);
    vTaskDelay(2500/portTICK_PERIOD_MS);
  
    char command2[50] = "AT+KUDPCFG=1,0";
    len = strnlen(command2, 100);
    command2[len] = '\r';
    write(atfd, command2, len+1);

    vTaskDelay(2500/portTICK_PERIOD_MS);
    char command3[100] = "AT+KUDPSND=1,\"54.189.156.244\",3333,50";
    len = strnlen(command3, 100);
    command3[len] = '\r';
    write(atfd, command3, len+1);


    vTaskDelay(2500/portTICK_PERIOD_MS);
    char command4[100] = "go suck my dick dude \n --EOF--Pattern--";
    len = strnlen(command4, 100);
    command4[len] = '\r';
    write(atfd, command4, len+1);
 
    vTaskDelay(2500/portTICK_PERIOD_MS);
    char command5[100] = "AT+KUDPRCV=1,17";
    len = strnlen(command5, 100);
    command5[len] = '\r';
    write(atfd, command5, len+1);

    vTaskDelay(250000/portTICK_PERIOD_MS);
  }
}

void spawn_uart_thread(){
  atfd = open( "/dev/ttyUSB0", O_RDWR);
  if (atfd == -1){
     ESP_LOGE(TAG, "Failed to open UART PORT!"); 
     ASSERT(0);
  }

  xTaskCreate(at_parser, "AT_PARSER", 2048, NULL, 5, NULL); 
  xTaskCreate(at_writter, "AT_WRITTER", 2048, NULL, 5, NULL); 
}
