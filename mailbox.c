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

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static const char         TAG[] = "MAILBOX";
static EventGroupHandle_t event_group;
static SemaphoreHandle_t  mailbox_sem;
/**********************************************************
*                                               FUNCTIONS *
**********************************************************/
void create_mailbox_freertos_objects(){
  event_group = xEventGroupCreate();
  mailbox_sem = xSemaphoreCreateMutex();

  ASSERT(event_group);
  ASSERT(mailbox_sem);
}

bool mailbox_wait(EventBits_t wait_bits) {
  if (wait_bits != MAILBOX_WAIT_READY     &&
      wait_bits != MAILBOX_WAIT_CONNECT   &&
      wait_bits != MAILBOX_WAIT_PROCESSED &&
      wait_bits != MAILBOX_WAIT_CONSUME ) {
    
    ESP_LOGE(TAG, "Unknown bits posted! (%d)", (int)wait_bits);
    ASSERT(0);
  }  


  // Don't care about current state - clear it!
  xEventGroupClearBits(event_group, wait_bits);

  ESP_LOGI(TAG, "Waiting for event (%d)", wait_bits);
  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      wait_bits,
                      pdTRUE,          // clear bit on exit
                      pdFALSE,         // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );
 
  if (uxBits == wait_bits){
    ESP_LOGI(TAG, "Event group set! (%d)", (int)wait_bits);
    return true;
  } else {
    ESP_LOGI(TAG, "Event group TO %d", (int)wait_bits);
    return false;
  } 
}

bool mailbox_post(EventBits_t post_bits){
  if (post_bits != MAILBOX_POST_READY     &&
      post_bits != MAILBOX_POST_CONNECT   &&
      post_bits != MAILBOX_POST_PROCESSED &&
      post_bits != MAILBOX_POST_CONSUME ) {
    
    ESP_LOGE(TAG, "Unknown bits posted!");
    ASSERT(0);
  }  

  xEventGroupSetBits(event_group, post_bits);
}

void get_mailbox_sem(){
   if (pdTRUE != xSemaphoreTake(mailbox_sem, MUTEX_TIMEOUT_PERIOD)) {
       ESP_LOGE(TAG, "FAILED TO TAKE mailbox_sem!");
       ASSERT(0);
   }
}

void put_mailbox_sem(){
    xSemaphoreGive(mailbox_sem);
}


void a(void * arg){
  mailbox_wait(MAILBOX_WAIT_CONSUME);
  puts("done!");
  vTaskDelay(1000000);
}

void b(void * arg){
  vTaskDelay(100);
  mailbox_post(MAILBOX_POST_READY);

  puts("oisted");
  vTaskDelay(1000000);
}



void mailbox_test(){
  puts("Starting mailbo");
  create_mailbox_freertos_objects();

  xTaskCreate(a, "", 1024, "", 5, NULL); 
  xTaskCreate(b, "", 1024, "", 5, NULL); 
}
