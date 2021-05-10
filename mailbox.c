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
  EventBits_t uxBits = xEventGroupWaitBits(event_group,
                      EVENT_GROUP_BITS,
                      pdTRUE,          // clear bit on exit
                      pdTRUE,          // Just wait for 1 bit ( we only have 1 bit)
                      EVENT_WAIT_PERIOD
      );
 
  if (uxBits == EVENT_GROUP_BITS){
    ESP_LOGI(TAG, "Event group set!");
    return true;
  } else {
    ESP_LOGI(TAG, "Event group TO");
    return false;
  }                 
}

bool wait_mail_ring(){
  xEventGroupSetBits( event_group, EVENT_GROUP_BITS);
}

void driver_a(void * arg){
  wait_mail_box();
}

void driver_b(void * arg){
  vTaskDelay(10);
  wait_mail_ring();
}

void testDriver(){
  create_mailbox_freertos_objects();
  xTaskCreate(driver_a, "", 1024, NULL, 5, NULL);
  xTaskCreate(driver_b, "", 1024, NULL, 5, NULL);
}
