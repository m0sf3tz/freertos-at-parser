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
static void debug_print_wait(bool set, int wait_bits){
  if (wait_bits != MAILBOX_WAIT_READY     &&
      wait_bits != MAILBOX_WAIT_CONNECT   &&
      wait_bits != MAILBOX_WAIT_WRITE     &&
      wait_bits != MAILBOX_WAIT_PROCESSED &&
      wait_bits != MAILBOX_WAIT_CONSUME ) {
    
    ESP_LOGE(TAG, "Unknown bits posted! (%d)", (int)wait_bits);
    ASSERT(0);
  }
  char to_s[] = "TIMEOUT!";
  char set_s[] = "SET!";

  switch(wait_bits){
    case(MAILBOX_WAIT_READY):
      if(set) 
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_READY", set_s);
     else
       ESP_LOGI(TAG, "%s: MAILBOX_WAIT_READY", to_s); 
      break;
    case(MAILBOX_WAIT_CONNECT):
      if (set)
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_CONNECT", set_s); 
      else 
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_CONNECT", to_s); 
      break;
    case(MAILBOX_WAIT_WRITE):
      if (set)
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_WRITE", set_s); 
      else 
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_WRITE", to_s); 
      break;
    case(MAILBOX_WAIT_PROCESSED):
      if(set)
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_PROCESSED", set_s);
       else
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_PROCESSED ", to_s); 
      break;
    case(MAILBOX_WAIT_CONSUME):
      if (set)
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_CONSUME", set_s); 
      else
        ESP_LOGI(TAG, "%s: MAILBOX_WAIT_CONSUME", to_s); 
      break;
  }
}

static void debug_print_post(int wait_bits){
  if (wait_bits != MAILBOX_POST_READY     &&
      wait_bits != MAILBOX_POST_CONNECT   &&
      wait_bits != MAILBOX_POST_WRITE     &&
      wait_bits != MAILBOX_POST_PROCESSED &&
      wait_bits != MAILBOX_POST_CONSUME ) {
    
    ESP_LOGE(TAG, "Unknown bits posted! (%d)", (int)wait_bits);
    ASSERT(0);
  }

  switch(wait_bits){
    case(MAILBOX_POST_READY):
      ESP_LOGI(TAG, "POSTING MAILBOX_POST_READY");
      break;
    case(MAILBOX_POST_CONNECT):
      ESP_LOGI(TAG, "POSTING MAILBOX_POST_CONNECT");
      break;
    case(MAILBOX_POST_WRITE):
      ESP_LOGI(TAG, "POSTING MAILBOX_POST_WRITE");
      break;
    case(MAILBOX_WAIT_PROCESSED):
      ESP_LOGI(TAG, "POSTING MAILBOX_POST_PROCESSED ");
      break;
    case(MAILBOX_WAIT_CONSUME):
      ESP_LOGI(TAG, "POSTING MAILBOX_POST_CONSUME");
      break;
  }
}

void create_mailbox_freertos_objects(){
  event_group = xEventGroupCreate();
  mailbox_sem = xSemaphoreCreateMutex();

  ASSERT(event_group);
  ASSERT(mailbox_sem);
}

bool mailbox_wait(EventBits_t wait_bits) {
  if (wait_bits != MAILBOX_WAIT_READY     &&
      wait_bits != MAILBOX_WAIT_CONNECT   &&
      wait_bits != MAILBOX_WAIT_WRITE     &&
      wait_bits != MAILBOX_WAIT_PROCESSED &&
      wait_bits != MAILBOX_WAIT_CONSUME ) {
    
    ESP_LOGE(TAG, "Unknown bits posted! (%d)", (int)wait_bits);
    ASSERT(0);
  }  

  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      wait_bits,
                      pdTRUE,          // dont bit on exit (if multiple bits are set we want to handle them all)
                      pdFALSE,         // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );

  if (uxBits == wait_bits){
    debug_print_wait(true, (int)wait_bits);
    return true;
  } else {
    debug_print_wait(false, (int)wait_bits);
    return false;
  } 
}

bool mailbox_post(EventBits_t post_bits){
  if (post_bits != MAILBOX_POST_READY     &&
      post_bits != MAILBOX_POST_CONNECT   &&
      post_bits != MAILBOX_POST_WRITE     &&
      post_bits != MAILBOX_POST_PROCESSED &&
      post_bits != MAILBOX_POST_CONSUME ) {
    
    ESP_LOGE(TAG, "Unknown bits posted!");
    ASSERT(0);
  }  
  
  debug_print_post(post_bits);
  xEventGroupSetBits(event_group, post_bits);
  return true;
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
  mailbox_wait(MAILBOX_WAIT_READY);
  puts("done!");
  vTaskDelay(1000000);
}

void b(void * arg){
  vTaskDelay(100);
  mailbox_post(MAILBOX_POST_CONSUME);
  mailbox_post(MAILBOX_WAIT_READY);

  puts("oisted");
  vTaskDelay(1000000);
}



void mailbox_test(){
  puts("Starting mailbo");
  create_mailbox_freertos_objects();

  xTaskCreate(a, "", 1024, "", 5, NULL); 
  xTaskCreate(b, "", 1024, "", 5, NULL); 
}
