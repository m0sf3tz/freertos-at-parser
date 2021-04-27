/*
 * FreeRTOS V202012.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 *******************************************************************************
 * NOTE 1: The POSIX port is a simulation (or is that emulation?) only!  Do not
 * expect to get real time behaviour from the POSIX port or this demo
 * application.  It is provided as a convenient development and demonstration
 * test bed only.
 *
 * Linux will not be running the FreeRTOS simulator threads continuously, so
 * the timing information in the FreeRTOS+Trace logs have no meaningful units.
 * See the documentation page for the Linux simulator for an explanation of
 * the slow timing:
 * https://freertos-wordpress.corp.amazon.com/FreeRTOS-simulator-for-Linux.html
 * - READ THE WEB DOCUMENTATION FOR THIS PORT FOR MORE INFORMATION ON USING IT -
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the comprehensive test and demo version.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * full demo.  Generic functions, such FreeRTOS hook functions, are defined in
 * main.c.
 *******************************************************************************
 *
 * main() creates all the demo application tasks, then starts the scheduler.
 * The web documentation provides more details of the standard demo application
 * tasks, which provide no particular functionality but do provide a good
 * example of how to use the FreeRTOS API.
 *
 * In addition to the standard demo tasks, the following tasks and tests are
 * defined and/or created within this file:
 *
 * "Check" task - This only executes every five seconds but has a high priority
 * to ensure it gets processor time.  Its main function is to check that all the
 * standard demo tasks are still operational.  While no errors have been
 * discovered the check task will print out "OK" and the current simulated tick
 * time.  If an error is discovered in the execution of a task then the check
 * task will print out an appropriate error message.
 *
 */


/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/* Kernel includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

/* Standard demo includes. */
#include "BlockQ.h"
#include "integer.h"
#include "semtest.h"
#include "PollQ.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "recmutex.h"
#include "flop.h"
#include "TimerDemo.h"
#include "countsem.h"
#include "death.h"
#include "dynamic.h"
#include "QueueSet.h"
#include "QueueOverwrite.h"
#include "EventGroupsDemo.h"
#include "IntSemTest.h"
#include "TaskNotify.h"
#include "QueueSetPolling.h"
#include "StaticAllocation.h"
#include "blocktim.h"
#include "AbortDelay.h"
#include "MessageBufferDemo.h"
#include "StreamBufferDemo.h"
#include "StreamBufferInterrupt.h"
#include "MessageBufferAMP.h"
#include "console.h"

#include "state_core.h"
#include "net_state.h"
#include "global_defines.h"


#define NEW_LINE_DELIMITER   (0)  // Found an EOL
#define NO_DELIMITER      (1)  // No EOL character
#define PARTIAL_DELIMETER_SCANNING (2)  // Partial EOL [returned when partial ---EOF--Pattern detected]
#define LONG_DELIMITER_FOUND (3) // returned when full long (--EOF...) delimiter found

#define BUFF_SIZE         (2048)
#define MAX_LINE_SIZE     (1024)
#define MAX_QUEUED_ITEMS  (2)
#define LONG_DELIMITER_LEN (19) // 19 == len(--EOF--Patter---)

static uint8_t   line_found[MAX_LINE_SIZE];
static uint8_t   buffer[BUFF_SIZE];
QueueSetHandle_t line_feed_q;

typedef struct {                                     
     uint16_t len;
     uint8_t  buf[MAX_LINE_SIZE];
} new_line_t;

static new_line_t new_line;

void digest_lines(uint8_t* line, size_t len){
  char fakebuf[100];
  memset(fakebuf, 0, 100);
  memcpy(fakebuf, line, len);
  printf("new line (len = %d), : %s \n", len, fakebuf);
}

// Hunts for two "EOL" delimiters
//  1) '\n' [ for normal URC, response]
//  2) '---EOF---Pattern---' , for TCP/UDP data pushes
int at_parser_delimiter_hunter(const uint8_t c){
  static char long_del[LONG_DELIMITER_LEN] = "---EOF---Pattern---";
  static char current_hunt[LONG_DELIMITER_LEN];
  static int iter = 0; 

  if ( c == '\n'){
    ESP_LOGI(TAG, "Found new-line delimiter");
    memset(current_hunt, 0, sizeof(current_hunt));
    iter = 0;
    return NEW_LINE_DELIMITER;  
  }

  current_hunt[iter] = c;
  int rc = memcmp(long_del, current_hunt, iter + 1); //must test at least one character
  if (rc == 0) {
    iter++;
    if (iter == LONG_DELIMITER_LEN){
      iter = 0;
      ESP_LOGI(TAG, "Found new long delimiter!");
      return LONG_DELIMITER_FOUND;
    } else {
      return PARTIAL_DELIMETER_SCANNING;
    }
  } else {
    iter = 0;
  }

  return NO_DELIMITER;
}

void at_parser_main(void * pv){
  static int iter_lead; // Reads ahead until all of end delimiter is hit 
  static int iter_lag;  // Lags behind while iter_lead hunts for EOL delimiter
  static int len;       // current length of line being parsed
  static bool found_line;

  ESP_LOGI(TAG, "Starting AT parser!");
  while(true){
    if(iter_lead == len){ // exhausted current buffer 
      // two sub cases
      // -> Failed to find EOL, exhausted current buffer
      // -> Still hunting for EOL
      BaseType_t xStatus =  xQueueReceive(line_feed_q, &new_line, portMAX_DELAY);
      if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to RX line!");
        ASSERT(0);
      }
      
      if (found_line){ 
        len = len - iter_lead;
        found_line = false;
      }

      if (len < 0){
        ESP_LOGE(TAG, "Negative len!!");
        ASSERT(0);
      }

      if(len == 0){
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, new_line.buf, new_line.len);
        len = new_line.len;
        iter_lead = 0;
        iter_lag = 0;
      } else {
        if ( len + new_line.len > BUFF_SIZE){
          ESP_LOGE(TAG, "Exceeded buffer size... error! Resetting!");
          memset(buffer, 0, sizeof(buffer));
          iter_lag = 0;
          iter_lead = 0;
          continue;
        }
        iter_lead++;
        iter_lag = iter_lead;

        memcpy(buffer + iter_lead, new_line.buf, new_line.len);
        len = len + new_line.len;
      }
    }
  
    if (iter_lead > MAX_LINE_SIZE){
       ESP_LOGE(TAG, "Exceeded buffer size... error! Resetting!");
       memset(buffer, 0, sizeof(buffer));
       iter_lag = 0;
       iter_lead = 0;
       abort();
       continue;
    }

    const int parse_status = at_parser_delimiter_hunter(buffer[iter_lead]);
    printf("%d, len = %d, iter_lead = %d \n", parse_status, len, iter_lead);
    
    if ( NEW_LINE_DELIMITER == parse_status ){
      // -1 strips the newline character
      memcpy(line_found, buffer, iter_lead); 
      digest_lines(line_found, iter_lead);
      memcpy(buffer, buffer + iter_lead + 1,  len - iter_lead);
      len = len - iter_lead - 1; // iter_lead "zero" based, len not 
      iter_lead = 0;
      iter_lag = 0;
      found_line = true;
    } else if (LONG_DELIMITER_FOUND == parse_status){
      memcpy(line_found, buffer, iter_lag);
      digest_lines(line_found, iter_lag);
      memcpy(buffer, buffer + iter_lead + 1,  len - iter_lead);
      len = len - iter_lead - 1; // iter_lead "zero" based, len not 
      iter_lead = 0;
      iter_lag = 0;
      found_line = true;
    }else if ( PARTIAL_DELIMETER_SCANNING == parse_status ) {
      iter_lead++;  
    } else {
      // neither a partial or complete delimiter found
      iter_lag++;
      iter_lead++;
    }
  }
}

void init_parser_freertos_objects(){
    line_feed_q = xQueueCreate(MAX_QUEUED_ITEMS, 500); 
    ASSERT(line_feed_q);
}

void driver (void * arg){
  ESP_LOGI(TAG, "Starting AT driver");
  static char foo[512] = "go such a dick! \n";
  new_line_t bar;
 
  bar.len = 1;
  //memcpy(bar.buf, "\n", 1);
  bar.buf[0] = 0;
  BaseType_t xStatus = xQueueSendToBack(line_feed_q, &bar, 0);
  xStatus = xQueueSendToBack(line_feed_q, &bar, 0);
  
    if (xStatus != pdTRUE) {
      ESP_LOGE(TAG, "Failed to send on event queue (net)");
      ASSERT(0);
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  
  
   vTaskDelete(NULL);
}


void spawn_uart_thread();

int main_full( void )
{
  //state_core_spawner();
  //net_state_spawner();

  xTaskCreate ( at_parser_main, "test", 2048, NULL, 4, NULL);
  
  init_parser_freertos_objects();
  
  xTaskCreate ( driver, "test", 2048, NULL, 4, NULL);
  //spawn_uart_thread();
  vTaskStartScheduler();
}
	
