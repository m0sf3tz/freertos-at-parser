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

  at_modem_respond_e term; 
  int cme_err =0;
  int len = 0;
  int line = 0;
  TickType_t start = xTaskGetTickCount();
  TickType_t end = start + PARSER_WAIT_FOR_UART;
  command_e cmd;
  at_parsed_s * parsed_p = get_parsed_struct();
  clear_at_parsed_struct();
  
  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    if(!buff){
      if (xTaskGetTickCount() > end){
        ESP_LOGE(TAG, "Failed to AT response!");
        parsed_p->status = AT_PROCESSED_TIMEOUT;

        mailbox_post(MAILBOX_POST_PROCESSED);
        ESP_LOGI(TAG, "watiing for done!");
        
        mailbox_wait(MAILBOX_WAIT_CONSUME);
        ESP_LOGI(TAG, "done comsuing "); 
        return parser_idle_state;  
      }
    }
    if(buff)
    {
        printf("%d ECHO len \n", len);
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
        if (get_net_state_cmd() == parsed_p->type){
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
      
      term = is_status_line(buff, len, &cme_err); 
      if(term){
        parsed_p->token  = get_net_state_token();
        parsed_p->status = AT_PROCESSED_GOOD;
        parsed_p->response = term;

        ESP_LOGI(TAG, "Done parsing! (len == %d)", len);
        ESP_LOGI(TAG, "ready to consume"); 
        
        mailbox_post(MAILBOX_POST_PROCESSED);
        ESP_LOGI(TAG, "watiing for done!");
        
        mailbox_wait(MAILBOX_WAIT_CONSUME);
        ESP_LOGI(TAG, "done comsuing "); 
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
  ESP_LOGI(TAG, "entering handle_write!");
  mailbox_post(MAILBOX_POST_READY); 

  at_modem_respond_e term; 
  int cme_err =0;
  int len = 0;
  int line = 0;
  command_e cmd;
  at_parsed_s * parsed_p = get_parsed_struct();
  clear_at_parsed_struct();
  
  TickType_t start = xTaskGetTickCount();
  TickType_t end = start + PARSER_WAIT_FOR_UART;
 

  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    if(!buff){
      if (xTaskGetTickCount() > end){
        ESP_LOGE(TAG, "Failed to AT response!");
        parsed_p->status = AT_PROCESSED_TIMEOUT;

        mailbox_post(MAILBOX_POST_PROCESSED);
        ESP_LOGI(TAG, "watiing for done!");
        
        mailbox_wait(MAILBOX_WAIT_CONSUME);
        ESP_LOGI(TAG, "done consuming (timeout) "); 
        return parser_idle_state;  
      }
    }
    if(buff)
    {
        printf("%d ECHO len \n", len);
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
        if (get_net_state_cmd() == parsed_p->type){
          break;
        } else {
          ESP_LOGE(TAG, "State machine out of sync - unexpected command! (%d)", parsed_p->type);
          ASSERT(0);
        }
      } else {
          ESP_LOGE(TAG, "State machine out of sync!");
          ASSERT(0);
      }
    }
  }

  // read Connect
  uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
  if (!buff){
    ASSERT(0);
  }
    
  if (is_connect_line(buff, len) == true){
      mailbox_post(MAILBOX_POST_CONNECT);
  } else {
    printf("%s asfasdf \n", buff);
     ESP_LOGE(TAG, "did not find connect string!");
     ASSERT(0);
  }
 
  mailbox_wait(MAILBOX_WAIT_WRITE);
  puts(" got write in  parser!");

  // read termination
  start = xTaskGetTickCount();
  end   = start + 1000;
  for(;;){
    buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    if(!buff){
       vTaskDelay(100/portMAX_DELAY);
       if (xTaskGetTickCount() > end){
         ESP_LOGE(TAG, "Timeout!");
         ASSERT(0);
       }
       continue;
    } else {
      break;
    }
  }

  printf ("status = %d \n", is_status_line(buff, len, cme_err) );
  mailbox_post(MAILBOX_POST_PROCESSED);

  return parser_idle_state;  
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

    if (*curr_state == parser_idle_state) {
        if (event == EVENT_ISSUE_WRITE) {
            ESP_LOGI(TAG, "Old State: parser_idle_state, Next: parser_handle_write");
            *curr_state = parser_handle_write_state;
            return;
        }
    }
    
}

static char* event_print_func(state_event_t event) {
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
