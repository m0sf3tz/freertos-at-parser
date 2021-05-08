#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>

#include "global_defines.h"
#include "parser_state.h"
#include "state_core.h"
#include "at_parser.h"

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "NET_STATE";
static SemaphoreHandle_t parser_state_mutex;
static bool              parser_state;
QueueSetHandle_t         events_parser_q;

/**********************************************************
*                                    FORWARD DECLARATIONS *
**********************************************************/

/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static state_t state_idle() {
  ESP_LOGI(TAG, "Entering Idle state");
/*
  if (at_incomming_peek() ) {
    // Data availible
    return EVENT_NEW_UART_DATA_F; 
  }
  */
  return NULL_STATE;
}

static state_t state_handle_urc() {
  ESP_LOGI(TAG, "Handling URC!");
  

}

static state_t state_wait_for_provisions_func() {
  return NULL_STATE;
}

// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
#if 0
  if (!curr_state) {
        ESP_LOGE(TAG, "ARG= NULL!");
        ASSERT(0);
    }

    if (*curr_state == parser_waiting_wifi) {
        if (event == EVENT_A) {
            ESP_LOGI(TAG, "Old State: parser_waiting_wifi, Next: parser_waiting_prov");
            *curr_state = parser_waiting_prov;
            return;
        }
    }

    if (*curr_state == parser_waiting_prov) {
        if (event == EVENT_B) {
            ESP_LOGI(TAG, "Old State: parser_waitin_prov, Next: parser_waiting_wifi");
            *curr_state = parser_waiting_wifi;
        }
    }

    // Stay in the same state
#endif 
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
  static state_array_s func_table[parser_state_len] = {
#if 0
        { state_wait_for_wifi_func      ,        portMAX_DELAY      },
        { state_wait_for_provisions_func, 2500 / portTICK_PERIOD_MS },
#endif 
    };
    if (state >= parser_state_len) {
        ESP_LOGE(TAG, "Current state out of bounds!");
        ASSERT(0);
    }

    return func_table[state];
}

/*********************************************************
*                NON-STATE  FUNCTIONS
**********************************************************/

static void parser_state_init_freertos_objects() {
    parser_state_mutex = xSemaphoreCreateMutex();
    events_parser_q    = xQueueCreate(EVENT_QUEUE_MAX_DEPTH, sizeof(state_event_t)); // state-core     -> parser-sm

    // make sure we init all the rtos objects
    ASSERT(parser_state_mutex);
    ASSERT(events_parser_q);
}

static bool event_filter_func(state_event_t event) {
#if 0
  switch(event){
      case(
    }
#endif 
}

static state_init_s* get_parser_state_handle() {
    static state_init_s parser_state = {
        .next_state        = next_state_func,
        .translator        = get_state_func,
        .event_print       = event_print_func,
        .starting_state    = parser_waiting_wifi,
        .state_name_string = "parser_state",
        .filter_event      = event_filter_func,
    };
    return &(parser_state);
}

void parser_state_spawner() {
    parser_state_init_freertos_objects();

    // State the state machine
    start_new_state_machine(get_parser_state_handle());
}


//// all bellow is test, you can delete!
//
static void driver (void * arg){
  bool status;
  int size; 
  for(;;){
    char * line = at_parser_main(false, &status, &size);

    if(status){
      at_digest_lines(line,size);
    } else {
      ESP_LOGE(TAG, "Found nothing!");
    }
  }
}

void parser_state_test(){
  printf("doing a test \n");
  xTaskCreate(driver, "", 3000, NULL, 3, NULL);
  vTaskStartScheduler();
}

