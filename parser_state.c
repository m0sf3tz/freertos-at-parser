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

/**********************************************************
*                                    FORWARD DECLARATIONS *
**********************************************************/
static state_t state_idle();
static state_t state_urc_handle();
static state_t state_handle_cmd_start();
static state_t state_handle_cmd();
static state_t state_handle_write_start();
static state_t state_handle_write();

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "NET_STATE";
static SemaphoreHandle_t parser_state_mutex;

// Translation Table
static state_array_s parser_translation_table[parser_state_len] = {
       { state_idle               ,  1000},
       { state_urc_handle         ,  0   }, 
       { state_handle_cmd_start   ,  portMAX_DELAY }, 
       { state_handle_cmd         ,  portMAX_DELAY }, 
       { state_handle_write       ,  portMAX_DELAY }, 
       { state_handle_write_start ,  portMAX_DELAY }, 
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

  vTaskDelay(100);
  if(buff){
     at_digest_lines(buff, len);
     //parse_at_string(buff, len, true, false);
  }
  
  return parser_idle_state; 
}

static state_t state_handle_cmd_start () {
  ESP_LOGI(TAG, "entering handle_cmd_start!");
  
  int len = 0;
  bool status;
  int cme_err;
  
  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &status, &len);
    printf("%d ECHO len \n", len);
    if(buff)
    {
      if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Error - unexpected status line!");
        return parser_idle_state;
        //TODO: handle  
      }

      if(is_urc(buff, len)){
        //TODO: handle URC
      }

      if( !at_line_explode(buff,len, 0)){
        //good! 
        return parser_handle_cmd_state;
      } else {
        //TODO: handle error!
      }
    }
  }
  return NULL_STATE;
}

static state_t state_handle_cmd () {
  ESP_LOGI(TAG, "entering handle_cmd!");
  mailbox_post(MAILBOX_POST_READY); 

  int cme_err =0;
  int len = 0;
  int line = 0;
  bool status = false;

  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &status, &len);
    printf("%d ECHO len \n", len);
    if(buff)
    {
      if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Error - unexpected status line!");
        return parser_idle_state;
        //TODO: handle  
      }

      if(is_urc(buff, len)){
        //TODO: handle URC
      }

      if( !at_line_explode(buff,len, 0)){
        // Foudn new command
        break;
      } else {
        //TODO: handle error!
      }
    }
  }

  // parse the lines of a command (not including first)
  for(line =1; line < MAX_LINES_AT; line++)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &status, &len);
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

static state_t state_handle_write_start() {
  ESP_LOGI(TAG, "entering handle_write_start!");
#if 0 
  int len = 0;
  bool status;
  int cme_err;
  
  for(;;)
  {
    uint8_t* buff = at_parser_stringer(PARSER_CMD_DEL, &status, &len);
    printf("%d ECHO len \n", len);
    if(buff)
    {
      if(is_status_line(buff, len, &cme_err)){
        ESP_LOGI(TAG, "Error - unexpected status line!");
        return parser_idle_state;
        //TODO: handle  
      }

      if(is_urc(buff, len)){
        //TODO: handle URC
      }

      if( !at_line_explode(buff,len, 0)){
        if (true){
        return parser_handle_cmd_state;
        } else {
          // TODO: handle
        }
      } else {
        //TODO: handle error!
      }
    }
  }
  return NULL_STATE;
#endif 
}

static state_t state_handle_write() {

  if (at_incomming_peek() ) {
    // Data availible
    return parser_urc_state; 
  }

  return NULL_STATE;
}

// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
  if (!curr_state) {
        ESP_LOGE(TAG, "ARG= NULL!");
        ASSERT(0);
    }

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
*                NON-STATE  FUNCTIONS
**********************************************************/

static void parser_state_init_freertos_objects() {
    parser_state_mutex = xSemaphoreCreateMutex();

    // make sure we init all the rtos objects
    ASSERT(parser_state_mutex);
}

static bool event_filter_func(state_event_t event) {
  switch(event){
    case(EVENT_URC_F)      : return true; 
    case(EVENT_DONE_URC_F) : return true;
    case(EVENT_ISSUE_CMD)  : return true;
    case(EVENT_ISSUE_SEND) : return true;
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
  //parser_state_spawner();
  char test[] = "+CFUN: 1,2,3";
  int d = parse_at_string(test,9,PARSER_FULL_MODE, 0);
  printf("%d \n", d);
}

