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
static const char        TAG[] = "NET_STATE";
static SemaphoreHandle_t net_state_mutex;
static bool              net_state;
QueueSetHandle_t         events_net_q;

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

    if (*curr_state == net_waiting_wifi) {
        if (event == EVENT_A) {
            ESP_LOGI(TAG, "Old State: net_waiting_wifi, Next: net_waiting_prov");
            *curr_state = net_waiting_prov;
            return;
        }
    }

    if (*curr_state == net_waiting_prov) {
        if (event == EVENT_B) {
            ESP_LOGI(TAG, "Old State: net_waitin_prov, Next: net_waiting_wifi");
            *curr_state = net_waiting_wifi;
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
    static state_array_s func_table[net_state_len] = {
        //{      (state function)       , looper time },
        { state_wait_for_wifi_func, portMAX_DELAY },
        { state_wait_for_provisions_func, 2500 / portTICK_PERIOD_MS },
    };

    if (state >= net_state_len) {
        ESP_LOGE(TAG, "Current state out of bounds!");
        ASSERT(0);
    }

    return func_table[state];
}

static void send_event_func(state_event_t event) {
    BaseType_t xStatus = xQueueSendToBack(events_net_q, &event, NET_SATE_QUEUE_TO);
    if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send on event queue (net)");
        ASSERT(0);
    }
}

static state_event_t get_event_func(uint32_t timeout) {
    state_event_t new_event = INVALID_EVENT;
    xQueueReceive(events_net_q, &new_event, timeout);
    return new_event;
}

/*********************************************************
*                NON-STATE  FUNCTIONS
**********************************************************/

static void net_state_init_freertos_objects() {
    net_state_mutex = xSemaphoreCreateMutex();
    events_net_q    = xQueueCreate(EVENT_QUEUE_MAX_DEPTH, sizeof(state_event_t)); // state-core     -> net-sm

    // make sure we init all the rtos objects
    ASSERT(net_state_mutex);
    ASSERT(events_net_q);
}

static bool event_filter_func(state_event_t event) {
    return true;
}

static state_init_s* get_net_state_handle() {
    static state_init_s net_state = {
        .next_state        = next_state_func,
        //.send_event        = send_event_func,
        //.get_event         = get_event_func,
        .translator        = get_state_func,
        .event_print       = event_print_func,
        .starting_state    = net_waiting_wifi,
        .state_name_string = "net_state",
        .filter_event      = event_filter_func,
    };
    return &(net_state);
}

void net_state_spawner() {
    net_state_init_freertos_objects();

    // State the state machine
    start_new_state_machine(get_net_state_handle());
}


//// all bellow is test, you can delete!
//
static void driver (void * arg){
  ESP_LOGI(TAG, "Starting AT driver");
  net_state_spawner();
  
  vTaskDelay(2000/portTICK_PERIOD_MS);
  
  puts("posint EVNAA"); 
  state_post_event(EVENT_A);

  vTaskDelay(2000/portTICK_PERIOD_MS);
  
  puts("posint EVNAAB"); 
  state_post_event(EVENT_B);
}

void net_state_test(){
  printf("doing a test \n");
  xTaskCreate(driver, "", 3000, NULL, 3, NULL);
  vTaskStartScheduler();
}

