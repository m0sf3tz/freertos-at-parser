#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>

#include "global_defines.h"
#include "cellular_state.h"
#include "state_core.h"

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "NET_STATE";
static SemaphoreHandle_t cellular_state_mutex;
static bool              cellular_state;
QueueSetHandle_t         events_cellular_q;

/**********************************************************
*                                    FORWARD DECLARATIONS *
**********************************************************/

/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static void state_wait_for_wifi_func() {
}

static void state_wait_for_provisions_func() {
}

// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
    if (!curr_state) {
        ESP_LOGE(TAG, "ARG= NULL!");
        ASSERT(0);
    }

    if (*curr_state == cellular_waiting_wifi) {
        if (event == EVENT_A) {
            ESP_LOGI(TAG, "Old State: cellular_waiting_wifi, Next: cellular_waiting_prov");
            *curr_state = cellular_waiting_prov;
            return;
        }
    }

    if (*curr_state == cellular_waiting_prov) {
        if (event == EVENT_B) {
            ESP_LOGI(TAG, "Old State: cellular_waitin_prov, Next: cellular_waiting_wifi");
            *curr_state = cellular_waiting_wifi;
        }
    }

    // Stay in the same state
}

static char* event_print_func(state_event_t event) {
#if 0 
  switch (event) {
    case (wifi_disconnect):
        return "wifi_disconnect";
        break;
    case (wifi_connect):
        return "wifi_connect";
        break;
    }
#endif
    // event not targeted at this state machine
    return NULL;
}

// Returns the state function, given a state
static state_array_s get_state_func(state_t state) {
    static state_array_s func_table[cellular_state_len] = {
        //{      (state function)       , looper time },
        { state_wait_for_wifi_func, portMAX_DELAY },
        { state_wait_for_provisions_func, 2500 / portTICK_PERIOD_MS },
    };

    if (state >= cellular_state_len) {
        ESP_LOGE(TAG, "Current state out of bounds!");
        ASSERT(0);
    }

    return func_table[state];
}

static void send_event_func(state_event_t event) {
    BaseType_t xStatus = xQueueSendToBack(events_cellular_q, &event, NET_SATE_QUEUE_TO);
    if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send on event queue (cellular)");
        ASSERT(0);
    }
}

static state_event_t get_event_func(uint32_t timeout) {
    state_event_t new_event = INVALID_EVENT;
    xQueueReceive(events_cellular_q, &new_event, timeout);
    return new_event;
}

/*********************************************************
*                NON-STATE  FUNCTIONS
**********************************************************/

static void cellular_state_init_freertos_objects() {
    cellular_state_mutex = xSemaphoreCreateMutex();
    events_cellular_q    = xQueueCreate(EVENT_QUEUE_MAX_DEPTH, sizeof(state_event_t)); // state-core     -> cellular-sm

    // make sure we init all the rtos objects
    ASSERT(cellular_state_mutex);
    ASSERT(events_cellular_q);
}

static bool event_filter_func(state_event_t event) {
    return true;
}

static state_init_s* get_cellular_state_handle() {
    static state_init_s cellular_state = {
        .next_state        = next_state_func,
        //.send_event        = send_event_func,
        //.get_event         = get_event_func,
        .translator        = get_state_func,
        .event_print       = event_print_func,
        .starting_state    = cellular_waiting_wifi,
        .state_name_string = "cellular_state",
        .filter_event      = event_filter_func,
    };
    return &(cellular_state);
}

void cellular_state_spawner() {
    cellular_state_init_freertos_objects();

    // State the state machine
    start_new_state_machine(get_cellular_state_handle());
}


//// all bellow is test, you can delete!
//
static void driver (void * arg){
  ESP_LOGI(TAG, "Starting AT driver");
  cellular_state_spawner();
  
  vTaskDelay(2000/portTICK_PERIOD_MS);
  
  puts("posint EVNAA"); 
  state_post_event(EVENT_A);

  vTaskDelay(2000/portTICK_PERIOD_MS);
  
  puts("posint EVNAAB"); 
  state_post_event(EVENT_B);
}

void cellular_state_test(){
  printf("doing a test \n");
  xTaskCreate(driver, "", 3000, NULL, 3, NULL);
  vTaskStartScheduler();
}

