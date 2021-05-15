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
#include "network_state.h"
#include "at_parser.h"
#include "mailbox.h"

/**********************************************************
*                                    FORWARD DECLARATIONS *
**********************************************************/
static state_t state_idle();
static state_t state_urc_handle();

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "NET_STATE";
network_state_s          net_context;
static  at_urc_parsed_s  urc_parsed;

// Translation Table
static state_array_s parser_translation_table[parser_state_len] = {
       { state_idle       ,  1000},
       { state_urc_handle ,  0   }, 
};


/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static state_t state_idle() {
}

static state_t state_urc_handle() {
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
}

static bool event_filter_func(state_event_t event) {
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


static void urc_hanlder(void * arg){
  for(;;){
    xQueueReceive(outgoing_urc_queue, &urc_parsed, portMAX_DELAY);
    ESP_LOGI(TAG, "Got a new URC!");
    print_parsed_urc(&urc_parsed);
  }
}


void driver_b(void * arg){

  puts("Test");

  net_context.curr_cmd = 3;
  //char str[] = "AT+KALTCFG=1,\"RRC_INACTIVITY_TIMER\"\r\n";
  char str[] = "AT+CESQ\r\n";
  int len = strlen(str);

  vTaskDelay(100);
  get_mailbox_sem();
  state_post_event(EVENT_ISSUE_CMD); 
  puts("issue");
  mailbox_wait(MAILBOX_WAIT_READY);
  puts("wait");

  at_command_issue_hal(str, len);
  mailbox_wait(MAILBOX_WAIT_PROCESSED);
  puts("posting consume");
  mailbox_post(MAILBOX_POST_CONSUME);
  puts("done!");
  put_mailbox_sem();

  vTaskDelay(1000000);
}

void network_driver(){
  char str[] = "+CEREG: 1\r\n";
  int len = strlen(str);
  verify_urc_and_parse(str, len);
  print_parsed_urc(get_urc_parsed_struct());
  
  //xTaskCreate(urc_hanlder, "", 1024, "", 5, NULL); 
  //xTaskCreate(driver_b, "", 1024, "", 5, NULL); 
}



