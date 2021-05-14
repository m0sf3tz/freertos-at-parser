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

bool mailbox_wait_for_ready() {
  // Don't care about current state - clear it!
  xEventGroupClearBits(event_group, MAILBOX_WAIT_FOR_READY);

  ESP_LOGI(TAG, "Waiting for mailbox ready!");
  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      MAILBOX_WAIT_FOR_READY,
                      pdTRUE,          // clear bit on exit
                      pdTRUE,          // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );
 
  if (uxBits == MAILBOX_WAIT_FOR_READY){
    ESP_LOGI(TAG, "Event group set! (%d)", uxBits);
    return true;
  } else {
    ESP_LOGI(TAG, "Event group TO %d", uxBits);
    return false;
  } 
}

bool mailbox_post_ready(){
  xEventGroupSetBits( event_group, MAILBOX_POST_READY);
}

bool mailbox_wait_for_work_request() {
  // Don't care about current state - clear it!
  xEventGroupClearBits(event_group, MAILBOX_WAIT_FOR_POST_WORK_REQUEST);

  ESP_LOGI(TAG, "Waiting for mailbox work request!");
  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      MAILBOX_WAIT_FOR_POST_WORK_REQUEST ,
                      pdTRUE,          // clear bit on exit
                      pdTRUE,          // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );
 
  if (uxBits == MAILBOX_WAIT_FOR_POST_WORK_REQUEST ){
    ESP_LOGI(TAG, "Event group set! (%d)", (int)uxBits);
    return true;
  } else {
    ESP_LOGI(TAG, "Event group TO %d", (int)uxBits);
    return false;
  } 
}

bool mailbox_post_work_request(){
  xEventGroupSetBits( event_group, MAILBOX_POST_WORK_REQUEST);
}

bool mailbox_wait_for_work_done() {
  // Don't care about current state - clear it!
  xEventGroupClearBits(event_group, MAILBOX_WAIT_FOR_WORK_DONE);

  ESP_LOGI(TAG, "Waiting for mailbox work done!");
  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      MAILBOX_WAIT_FOR_WORK_DONE,
                      pdTRUE,          // clear bit on exit
                      pdTRUE,          // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );
 
  if (uxBits == MAILBOX_WAIT_FOR_WORK_DONE){
    ESP_LOGI(TAG, "Event group set! (%d)", (int)uxBits);
    return true;
  } else {
    ESP_LOGI(TAG, "Event group TO %d", (int)uxBits);
    return false;
  } 
}

bool mailbox_post_work_done(){
  xEventGroupSetBits( event_group, MAILBOX_POST_WORK_DONE);
}

bool mailbox_wait_for_consume() {
  // Don't care about current state - clear it!
  xEventGroupClearBits(event_group, MAILBOX_WAIT_FOR_CONSUME);

  ESP_LOGI(TAG, "Waiting for mailbox ready!");
  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      MAILBOX_WAIT_FOR_CONSUME,
                      pdTRUE,          // clear bit on exit
                      pdTRUE,          // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );
 
  if (uxBits == MAILBOX_WAIT_FOR_CONSUME){
    ESP_LOGI(TAG, "Event group set! (%d)", uxBits);
    return true;
  } else {
    ESP_LOGI(TAG, "Event group TO %d", uxBits);
    return false;
  } 
}

bool mailbox_post_consumed(){
  xEventGroupSetBits( event_group, MAILBOX_POST_CONSUME);
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

void driver_a(void * arg){
  mailbox_wait_for_ready();
  mailbox_wait_for_work_request();
  mailbox_wait_for_work_done();
  mailbox_post_consumed();
}

void driver_b(void * arg){
  puts("here");
  vTaskDelay(100/portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "Posting ready!");
  mailbox_post_ready();
  vTaskDelay(10/portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "Posting work request!");
  mailbox_post_work_request();
  vTaskDelay(10);
  mailbox_post_work_done();
  vTaskDelay(10);
  mailbox_wait_for_consume();
}


void testDriver(){
  puts("Starting mailbox test");
  create_mailbox_freertos_objects();
  xTaskCreate(driver_a, "", 2024, NULL, 5, NULL);
  puts("Starting mailbox test");
  xTaskCreate(driver_b, "", 2024, NULL, 6, NULL);
  puts("Starting mailbox test");
}
