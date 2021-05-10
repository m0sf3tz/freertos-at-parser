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
static const char        TAG[] = "MAILBOX";
static EventGroupHandle_t event_group;
/**********************************************************
*                                               FUNCTIONS *
**********************************************************/
void post_work(){
  
}

void create_mailbox_freertos_objects(){
  event_group = xEventGroupCreate();
  ASSERT(event_group);
}

// Returns true if event set
bool wait_mail_box(){

  // Don't care about current state - clear it!
  xEventGroupClearBits(event_group, EVENT_GROUP_BITS);

  ESP_LOGI(TAG, "Waiting for mailbox ring!");
  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      EVENT_GROUP_BITS,
                      pdTRUE,          // clear bit on exit
                      pdTRUE,          // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );
 
  if (uxBits == EVENT_GROUP_BITS){
    ESP_LOGI(TAG, "Event group set! (%d)", uxBits);
    return true;
  } else {
    ESP_LOGI(TAG, "Event group TO %d", uxBits);
    return false;
  }                 
}

bool wait_mail_ring(){
  xEventGroupSetBits( event_group, EVENT_GROUP_BITS);
}

void driver_a(void * arg){
  for(;;){
    wait_mail_box();
  }
}

void driver_b(void * arg){
  for(;;){
  vTaskDelay(EVENT_WAIT_PERIOD + 4);
  puts("Ringing mailbox!");
  wait_mail_ring();
  }

}

void testDriver(){
  puts("Starting mailbox test");
  create_mailbox_freertos_objects();
  xTaskCreate(driver_a, "", 1024, NULL, 5, NULL);
  xTaskCreate(driver_b, "", 1024, NULL, 5, NULL);
}
