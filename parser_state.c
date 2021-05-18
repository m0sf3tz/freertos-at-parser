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
#include "mailbox.h"

/*********************************************************
*                                                GLOBALS *
*********************************************************/
QueueSetHandle_t  outgoing_urc_queue;

/*********************************************************
*                                   FORWARD DECLARATIONS *
*********************************************************/
static state_t state_idle_func();
static state_t state_handle_cmd_func();
static state_t state_handle_write_func();

static void    post_urc_to_network_layer(at_urc_parsed_s * ptr);

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "PARSER_STATE";
static SemaphoreHandle_t parser_state_mutex;

// Translation Table
static state_array_s parser_translation_table[parser_state_len] = {
       { state_idle_func          ,  300 },
       { state_handle_cmd_func    ,  portMAX_DELAY }, 
       { state_handle_write_func  ,  portMAX_DELAY }, 
};


/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static state_t state_idle_func() {
  ESP_LOGI(TAG, "Idle state!");
  int cme_err;
  int len;

  // read the new line
  uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL,  &len);
  if(buff == NULL){
    return NULL_STATE;
  }
  
  if(is_status_line(buff, len, &cme_err)){
     ESP_LOGI(TAG, "Error - unexpected status line!");
     return parser_idle_state;
     //TODO: handle  
   }

   if(verify_urc_and_parse(buff, len)){
      ESP_LOGI(TAG, "Found URC, handling");
      post_urc_to_network_layer(get_urc_parsed_struct());
      return NULL_STATE;
   } 
  
   ESP_LOGE(TAG, "unknown sequence followed!");
   ASSERT(0);
   //TODO: handle

   return NULL_STATE;
}

static state_t state_handle_cmd_func () {
  ESP_LOGI(TAG, "entering handle_cmd!");
  mailbox_post(MAILBOX_POST_READY); 

  int cme_err =0;
  int len = 0;
  int line = 0;
  command_e cmd;

  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    printf("%d ECHO len \n", len);
    if(buff)
    {
      if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Error - unexpected status line!");
        return parser_idle_state;
        //TODO: handle  
      }

      if(verify_urc_and_parse(buff, len)){
        continue;
      }

      if( !at_line_explode(buff,len, 0)){
        // verifiy we are dealing with the correct command
        at_parsed_s * parsed_p = get_parsed_struct();
        if (get_net_state_cmd() == parsed_p->type){
          // set the token, we will verify in network state
          parsed_p->token = get_net_state_token();
          break;
        } else {
          ESP_LOGE(TAG, "State machine out of sync - unexpected command! (%d)", parsed_p->type);
          ASSERT(0);
        }
      } else {
          ESP_LOGE(TAG, "State machine out of sync!");
          //ASSERT(0);
      }
    }
  }

  // parse the lines of a command (not including first)
  for(line =1; line < MAX_LINES_AT; line++)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    if (buff){
      if (len == 0){
         ESP_LOGE(TAG, "Failed to pars!");
        // TODO: handle...
      }
        
      if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Done parsing! (len == %d)", len);
        
        print_parsed();
        puts("ready to consume"); 
        
        mailbox_post(MAILBOX_POST_PROCESSED);
        puts("watiing for done!");
        
        mailbox_wait(MAILBOX_WAIT_CONSUME);
        puts("done comsuing "); 
        return parser_idle_state;  
      }

      if(at_line_explode(buff,len, 1)){
        ESP_LOGE(TAG, "Failed to pars!");
        // TODO: handle...
      } 
    }
  } 
  return NULL_STATE;
}

static state_t state_handle_write_func() {

  return NULL_STATE;
}

// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
    if (*curr_state == parser_idle_state) {
        if (event == EVENT_ISSUE_CMD) {
            ESP_LOGI(TAG, "Old State: parser_idle_state, Next: parser_handle_cmd_start_state");
            *curr_state = parser_handle_cmd_state;
            return;
        }
    }
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
                                    NON-STATE  FUNCTIONS *
*********************************************************/
static void post_urc_to_network_layer(at_urc_parsed_s * ptr){
  if(!ptr){
    ESP_LOGE(TAG, "NULL ARG!");
    ASSERT(0);
  }

  BaseType_t xStatus = xQueueSendToBack(outgoing_urc_queue, (void*)ptr, RTOS_DONT_WAIT);
  if(xStatus == pdFALSE){
    ESP_LOGE(TAG, "ran out of space in queue");
    ASSERT(0);
    //TODO - handle gracefully?
  }
}

static void parser_state_init_freertos_objects() {
    parser_state_mutex = xSemaphoreCreateMutex();
    outgoing_urc_queue = xQueueCreate(MAX_URC_OUTSTANDING, sizeof(at_urc_parsed_s)); // parser_state -> network_state 

    // make sure we init all the rtos objects
    ASSERT(parser_state_mutex);
    ASSERT(outgoing_urc_queue);
}

static bool event_filter_func(state_event_t event) {
  switch(event){
    case(EVENT_URC_F)       : return true; 
    case(EVENT_DONE_URC_F)  : return true;
    case(EVENT_ISSUE_CMD)   : return true;
    case(EVENT_ISSUE_WRITE) : return true;
    defaut                  : return false;
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
