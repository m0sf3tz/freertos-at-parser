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
#include "net_adaptor.h"

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
static state_t state_handle_read_func();

static void       post_urc_to_network_layer(at_urc_parsed_s * ptr);
static TickType_t get_cmd_timeout(command_e cmd);

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
       { state_handle_read_func   ,  portMAX_DELAY }, 
};

/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static state_t state_idle_func() {
  int cme_err;
  int len;

#ifndef FAKE_INPUT_STREAM_MODE
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
#endif 

   return NULL_STATE;
}

static state_t state_handle_cmd_func () {
  int cme_err            = 0;
  int len                = 0;
  int line               = 0;
  TickType_t echo_to     = xTaskGetTickCount() + PARSER_WAIT_FOR_ECHO;
  TickType_t cmd_to      = xTaskGetTickCount() + get_cmd_timeout( get_net_state_cmd() );
  at_parsed_s * parsed_p = get_parsed_struct();
  at_modem_respond_e term; 
  command_e cmd;
  
  clear_at_parsed_struct();
  
  ESP_LOGI(TAG, "entering handle_cmd!");
  mailbox_post(MAILBOX_POST_READY); 

  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    if(!buff){
      if (xTaskGetTickCount() > echo_to){
         ESP_LOGE(TAG, "Failed to get AT echo!");
         parsed_p->status = AT_PROCESSED_TIMEOUT;
         goto error;
      } else {
        vTaskDelay(50/portTICK_PERIOD_MS);
      }
    }
    if(buff)
    {
      if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Error - unexpected status line!");
        parsed_p->status = AT_PROCESSED_DERAIL;
        goto error;
      }

      if(verify_urc_and_parse(buff, len)){
        ESP_LOGI(TAG, "Waiting for cmd echo - got URC instead");
        post_urc_to_network_layer(get_urc_parsed_struct());
        continue;
      }

      if( at_line_explode(buff,len, 0) == 0 ){
        // verifiy we are dealing with the correct command
        if (get_net_state_cmd() == parsed_p->type){
          break;
        } else {
          ESP_LOGE(TAG, "State machine out of sync - unexpected command! (%d)", parsed_p->type);
          parsed_p->status = AT_PROCESSED_DERAIL;
          goto error;
        }
      } else {
          ESP_LOGE(TAG, "State machine out of sync!");
          parsed_p->status = AT_PROCESSED_DERAIL;
          goto error;
      }
    }
  }

  // parse the lines of a command (not including first)
  for(line = 1; line < MAX_LINES_AT;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    if (buff){
      if (len == 0){
         ESP_LOGE(TAG, "Read zero lenght!");
         ASSERT(0);
      }
      
      term = is_status_line(buff, len, &cme_err); 
      if(term){
        parsed_p->token     = get_net_state_token();
        parsed_p->status    = AT_PROCESSED_GOOD;
        parsed_p->response  = term;
        parsed_p->cme       = cme_err;
        parsed_p->num_lines = line - 1;

        ESP_LOGI(TAG, "Done parsing! (len == %d)", len);
        ESP_LOGI(TAG, "ready to consume"); 
        
        mailbox_post(MAILBOX_POST_PROCESSED);
        ESP_LOGI(TAG, "wating for done processing ACK!");
        
        mailbox_wait(MAILBOX_WAIT_CONSUME, MAILBOX_WAIT_TIME_NOMINAL);
        ESP_LOGI(TAG, "network state finished consuming"); 
        return parser_idle_state;  
      }

      if(at_line_explode(buff,len, line)){
        parsed_p->status = AT_PROCESSED_DERAIL;
        ESP_LOGE(TAG, "Failed to parse line!");
        goto error;
      }

      line++; 
    } else{
      if (xTaskGetTickCount() > cmd_to){
        parsed_p->status = AT_PROCESSED_TIMEOUT;
        ESP_LOGE(TAG, "Timeout on cmd!");
        goto error;
      }
      vTaskDelay(25/portTICK_PERIOD_MS);
    } 
  }
error:

  mailbox_post(MAILBOX_POST_PROCESSED);
  ESP_LOGI(TAG, "(fail) Posting Processed!");
        
  mailbox_wait(MAILBOX_WAIT_CONSUME, MAILBOX_WAIT_TIME_NOMINAL);
  ESP_LOGI(TAG, "(fail) finished consuming!"); 

  return parser_idle_state;
}

static state_t state_handle_read_func() {
  ESP_LOGI(TAG, "entering handle read!!");
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

  udp_packet_s udp;
  memset(&udp, 0, sizeof(udp_packet_s));

  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
    if(!buff){
      if (xTaskGetTickCount() > end){
        ESP_LOGE(TAG, "Failed to AT response! (timeout)");
        parsed_p->status = AT_PROCESSED_TIMEOUT;

        mailbox_post(MAILBOX_POST_PROCESSED);
        ESP_LOGI(TAG, "Waiting for done! (timeout)");
        
        mailbox_wait(MAILBOX_WAIT_CONSUME, MAILBOX_WAIT_TIME_NOMINAL);
        ESP_LOGI(TAG, "Done consuming (timeout) "); 
        return parser_idle_state;  
      }
    }
    if(buff)
    {
        if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Error - unexpected status line!");
        return parser_idle_state;
        //TODO: handle  
      }

      if(verify_urc_and_parse(buff, len)){
        post_urc_to_network_layer(get_urc_parsed_struct());
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


  // TODO: could this be improved? - need a loop?
  // read Connect
  uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &len);
  if (!buff){
    ASSERT(0);
  }
    
  if (is_connect_line(buff, len) == true){
      mailbox_post(MAILBOX_POST_CONNECT);
  } else {
     ESP_LOGE(TAG, "did not find connect string!");
     ASSERT(0);
  }
 
  // read termination
  start = xTaskGetTickCount();
  end   = start + 1000;
  for(;;){
    buff = at_parser_stringer(PARSER_DATA_DEL, &len);
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

  udp.len = len;
  memcpy(udp.data, buff, len);
  xQueueSendToBack(incoming_udp_q, &udp, RTOS_DONT_WAIT);

  //ESP_LOGI(TAG, "read --->%s", buff);
  parsed_p->status = AT_PROCESSED_GOOD;

  vTaskDelay(100/portMAX_DELAY);

  buff = at_parser_stringer(PARSER_CMD_DEL, &len);
  printf ("status = %d \n", is_status_line(buff, len, cme_err) );
  
  mailbox_post(MAILBOX_POST_PROCESSED);

  mailbox_wait(MAILBOX_WAIT_CONSUME, MAILBOX_WAIT_TIME_NOMINAL);
  put_mailbox_sem();

  return parser_idle_state;  
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
        
        mailbox_wait(MAILBOX_WAIT_CONSUME, MAILBOX_WAIT_TIME_NOMINAL);
        ESP_LOGI(TAG, "done consuming (timeout) "); 
        return parser_idle_state;  
      }
    }
    if(buff)
    {
        if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Error - unexpected status line!");
        return parser_idle_state;
        //TODO: handle  
      }

      if(verify_urc_and_parse(buff, len)){
        post_urc_to_network_layer(get_urc_parsed_struct());
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
     ESP_LOGE(TAG, "did not find connect string!");
     ASSERT(0);
  }
 
  mailbox_wait(MAILBOX_WAIT_WRITE, MAILBOX_WAIT_TIME_NOMINAL);
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
        if (event == EVENT_ISSUE_READ) {
            ESP_LOGI(TAG, "Old State: parser_idle_state, Next: parser_handle_read");
            *curr_state = parser_handle_read_state;
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
static TickType_t get_cmd_timeout(command_e cmd){
 switch(cmd){
  case CFUN: return CFUN_CMD_TIMEOUT;
  default: ASSERT(0);
  } 
}

static void post_urc_to_network_layer(at_urc_parsed_s * ptr){
  if(!ptr){
    ESP_LOGE(TAG, "NULL ARG!");
    ASSERT(0);
  }

  BaseType_t xStatus = xQueueSendToBack(outgoing_urc_queue, (void*)ptr, RTOS_DONT_WAIT);
  if(xStatus == pdFALSE){
    ESP_LOGE(TAG, "ran out of space in queue");
    ASSERT(0);
  }

  // reset structure! 
  memset(ptr, 0, sizeof(at_urc_parsed_s));
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
    case(EVENT_ISSUE_READ) : return true;
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
