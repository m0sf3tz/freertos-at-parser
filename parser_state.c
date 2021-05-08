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

/**********************************************************
*                                    FORWARD DECLARATIONS *
**********************************************************/
static state_t state_idle();
static state_t state_urc_handle();

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "NET_STATE";
static SemaphoreHandle_t parser_state_mutex;
QueueSetHandle_t         events_parser_q;

// Translation Table
static state_array_s parser_translation_table[parser_state_len] = {
       { state_idle       ,  1000},
       { state_urc_handle ,  0   }, 
};


/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static state_t state_idle() {
  ESP_LOGI(TAG, "Entering Idle state");

  if (at_incomming_peek() ) {
    // Data availible
    return parser_urc_state; 
  }

  return NULL_STATE;
}

static state_t state_urc_handle() {
  ESP_LOGI(TAG, "Handling URC!");

  int len = 0;
  bool status;
  uint8_t* buff = at_parser_stringer(false, &status, &len);

  if(len != -1){
    at_digest_lines(buff, len);
  }

  return parser_idle_state; 
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
  switch(event){
    case(EVENT_URC_F): return true; 
    case(EVENT_DONE_URC_F): return true;
  }
}

static state_init_s* get_parser_state_handle() {
    static state_init_s parser_state = {
        .next_state        = next_state_func,
        .translation_table = parser_translation_table,
        .event_print       = event_print_func,
        .starting_state    = parser_idle_state,
        .state_name_string = "parser_state",
        .filter_event      = event_filter_func,
        .total_states      = parser_state_len,  
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
    char * line = at_parser_stringer(false, &status, &size);

    if(status){
      at_digest_lines(line,size);
    } else {
      ESP_LOGE(TAG, "Found nothing!");
    }
  }
}

void parser_state_test(){
  parser_state_spawner();

  vTaskStartScheduler();
}

