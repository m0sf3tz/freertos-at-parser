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
static state_t state_detached();
static state_t state_attached();

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "NET_STATE";
network_state_s          net_context;
static  at_urc_parsed_s  urc_parsed;

// Translation Table
static state_array_s network_translation_table[network_state_len] = {
       { state_detached ,  portMAX_DELAY },
       { state_attached ,  portMAX_DELAY }, 
};


/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static state_t state_detached() {
  ESP_LOGI(TAG, "Entering detached state!");
  return NULL_STATE;
}

static state_t state_attached() {
  ESP_LOGI(TAG, "Entering attached state!");
  return NULL_STATE;
}


// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
  if (*curr_state == network_attaching_state) {
      switch(event){
        case (NETWORK_ATTACHED):
            ESP_LOGI(TAG, "detached->attached");
            *curr_state = network_idle_state;
            break;
      }
  }
  if (*curr_state == network_idle_state) {
      switch(event){
        case (NETWORK_DETACHED):
            ESP_LOGI(TAG, "attached->detached");
            *curr_state = network_attaching_state;
            break;
      }
  }

}

static char* event_print_func(state_event_t event) {
    return NULL;
}

static bool event_filter_func(state_event_t event) {
  switch(event){
    case(NETWORK_ATTACHED)   : return true; 
    case(NETWORK_DETACHED)   : return true; 
  }
}

/*********************************************************
*                NON-STATE  FUNCTIONS
**********************************************************/

static state_init_s* get_network_state_handle() {
    static state_init_s parser_state = {
        .next_state        = next_state_func,
        .translation_table = network_translation_table,
        .event_print       = event_print_func,
        .starting_state    = network_attaching_state,
        .state_name_string = "network_state",
        .filter_event      = event_filter_func,
        .total_states      = network_state_len,  
    };
    return &(parser_state);
}


static void urc_hanlder(void * arg){
  for(;;){
    xQueueReceive(outgoing_urc_queue, &urc_parsed, portMAX_DELAY);
    ESP_LOGI(TAG, "Got a new URC!");
    print_parsed_urc(&urc_parsed);

    switch(urc_parsed.type){
      case(CEREG):
        if (urc_parsed.param_arr[0].val == 1){
          ESP_LOGI(TAG, "Registered! - posting!");
          state_post_event(NETWORK_ATTACHED);
        } else if (urc_parsed.param_arr[0].val != 1){ //TODO: wrong! (roaming!)
          ESP_LOGI(TAG, "detached! - posting!");
          state_post_event(NETWORK_DETACHED);
        }
    }
  }
}


void driver_b(void * arg){

  // State the state machine
  start_new_state_machine(get_network_state_handle());
/*
  int r = 0;
  for(;;){
      vTaskDelay(10000/portTICK_PERIOD_MS);

      if (r & 1 == 1){
        char str[] = "AT+CFUN=0\r\n";
        int len = strlen(str);
        at_command_issue_hal(str, len);
      } else {
         puts("wtf?");
         char str[] = "AT+CFUN=1\r\n";
         int len = strlen(str);
         at_command_issue_hal(str, len);
      }
      r++;
      vTaskDelay(10000/portTICK_PERIOD_MS);
  }
*/


  puts("Test");
  int len;
  char str[300]; 
  memset(str, 0, 100);
  int r = 0;

  for(;;){
      if (r & 1 == 1){
        memcpy(str, "AT+CFUN=0\r\n", strlen("AT+CFUN=0\r\n"));
        len = strlen(str);

      } else {
        memcpy(str, "AT+CFUN=1\r\n", strlen("AT+CFUN=1\r\n"));
        len = strlen(str);
      }
      r++;

      vTaskDelay(10000/portTICK_PERIOD_MS);

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
     
      vTaskDelay(10000/portTICK_PERIOD_MS);
    }
  vTaskDelay(1000000);
}

void network_driver(){
  /*
  char str[] = "+CEREG: 1\r\n";
  int len = strlen(str);
  verify_urc_and_parse(str, len);
  print_parsed_urc(get_urc_parsed_struct());
  */
  xTaskCreate(urc_hanlder, "", 1024, "", 5, NULL); 
  xTaskCreate(driver_b, "", 1024, "", 5, NULL); 
  
}



