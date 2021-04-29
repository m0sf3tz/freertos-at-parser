#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>

#include "global_defines.h"
#include "net_state.h"
#include "state_core.h"

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "CELLULAR_CORE";
QueueSetHandle_t         issue_command_q;
static char              new_command_buff[MAX_WRITE_COMMAND_LEN];

/**********************************************************
*                                    FORWARD DECLARATIONS *
**********************************************************/

/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/

static void at_command_issue_thread(void * arg) {
    for (;;){
      memset(new_command_buff, 0, sizeof(new_command_buff));

      BaseType_t xStatus = xQueueReceive(issue_command_q, new_command_buff, portMAX_DELAY);
      if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to rx on at_command_issue");
        ASSERT(0);
      }


}

static void cellular_core_init_freertos_objects() {
    issue_command_q  = xQueueCreate(MAX_OUTSTANDING_WRITE_COMMANDS ,MAX_WRITE_COMMAND_LEN); 

    // make sure we init all the rtos objects
    ASSERT(issue_command_q);
}


void cellular_core_spawner() {
    cellular_core_init_freertos_objects();
}
