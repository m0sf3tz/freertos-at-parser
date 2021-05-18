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
#include "uart_core.h"
#include "threegpp.h"

/*********************************************************
*                                   FORWARD DECLARATIONS *
*********************************************************/
static state_t state_detached_func();
static state_t state_attached_func();
static state_t state_write_func();

static void set_net_state_cmd(command_e cmd);
static void set_net_state_token();

static bool send_cmd(uint8_t* cmd, int len, void (*clb)(void), command_e cmd_enum);

static void verify_kcnxfg();
static void verify_cereg();
/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char         TAG[] = "NET_STATE";
static network_state_s    net_context;
static at_urc_parsed_s    urc_parsed;
static SemaphoreHandle_t  network_state_mutex;
static char               misc_buff[MISC_BUFF_SIZE]; 

// Translation Table
static state_array_s network_translation_table[network_state_len] = {
       { state_detached_func,  portMAX_DELAY },
       { state_attached_func,  portMAX_DELAY }, 
       { state_write_func   ,  portMAX_DELAY }, 
};


/*********************************************************
*                                        STATE FUNCTIONS *
*********************************************************/
static state_t state_detached_func() {
  ESP_LOGI(TAG, "Entering detached state!");
  ESP_LOGI(TAG, "------------>ENTERING");
/*  
  memcpy(misc_buff, "AT+CEREG=2\r\n", strlen("AT+CEREG=2\r\n"));
  int len = strlen(misc_buff);
 
  //verify we are detatched 
  send_cmd(misc_buff, len, verify_cereg, CEREG);
*/
  return NULL_STATE;
}

static state_t state_attached_func() {
  ESP_LOGI(TAG, "Entering attached state!");

  //create_kcnxcfg_cmd(misc_buff, MISC_BUFF_SIZE);
  //send_cmd(misc_buff, strlen(misc_buff), verify_kcnxfg, CFUN);
  
  return NULL_STATE;
}


// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
  if (*curr_state == network_attaching_state) {
      switch(event){
        case (NETWORK_ATTACHED):
            ESP_LOGI(TAG, "detached->attached");
            *curr_state = network_attached_state;
            break;
      }
  }

  if (*curr_state == network_attached_state) {
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
    case(NETWORK_ATTACHED): return true; 
    case(NETWORK_DETACHED): return true; 
  }
  return false;
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

void dummy_callbuck(){
  puts("dumy callback");
  print_parsed();
}

static void verify_kcnxfg(){
  puts("dumy callback");
  print_parsed();
}

static void verify_cereg(){

  print_parsed();

  at_parsed_s *parsed = get_parsed_struct();
  if(parsed->type != CEREG){
    ESP_LOGE(TAG, "CMD type not CEREG -> (%d)", parsed->type);
    ASSERT(0);
  }
  if(parsed->form != READ_CMD ){
    ESP_LOGE(TAG, "form type not CEREG -> (%d)", parsed->form);
    ASSERT(0);
  }

  if (parsed->param_arr[CEREG_STATUS_LINE][CEREG_STATUS_INDEX].val == CEREG_CONNECTED){
    ESP_LOGI(TAG, "Registered! - posting!");
    state_post_event(NETWORK_ATTACHED);
  }  
}

static bool send_cmd(uint8_t* cmd, int len, void (*clb)(void), command_e cmd_enum){
  ASSERT(cmd);

  get_mailbox_sem();

  ESP_LOGI(TAG, "Posting issue cmd!");
  state_post_event(EVENT_ISSUE_CMD);

  ESP_LOGI(TAG, "waiting for ready from PARSER_STARTE!");
  if (!mailbox_wait(MAILBOX_WAIT_READY)) goto fail;
  set_net_state_cmd(cmd_enum);
  set_net_state_token();

  ESP_LOGI(TAG, "ISSUE CMD-> %s", cmd);
  if(at_command_issue_hal(cmd, len) == -1) goto fail;
  
  ESP_LOGI(TAG, "waiting for processed CMD from PARSER_STATE");
  if(!mailbox_wait(MAILBOX_WAIT_PROCESSED)) goto fail;
 
  // verify token
  at_parsed_s * parsed_p = get_parsed_struct();
  if (get_net_state_token() != parsed_p->token){
    ESP_LOGE(TAG, "TOKEN NOT CORRECT!");
    ASSERT(0);
  }

  ESP_LOGI(TAG, "Calling CALLBACK for send_cmd!");
  clb();

  if(!mailbox_post(MAILBOX_POST_CONSUME)) goto fail;

  put_mailbox_sem();
  return true;

fail:
  ESP_LOGE(TAG, "Failed to issue cmd!");
  put_mailbox_sem(); 
  return false;
}

static void network_state_init_freertos_objects() {
    network_state_mutex = xSemaphoreCreateMutex();

    // make sure we init all the rtos objects
    ASSERT(network_state_mutex);
}

static void network_state_init(){
  network_state_init_freertos_objects();

  // State the state machine
  start_new_state_machine(get_network_state_handle());
}

static void set_net_state_cmd(command_e cmd){
  if (pdTRUE != xSemaphoreTake(network_state_mutex, NET_MUTEX_WAIT)) {
      ESP_LOGE(TAG, "FAILED TO TAKE net mutext!");
      ASSERT(0);
  }
  net_context.curr_cmd = cmd;
  xSemaphoreGive(network_state_mutex);
}

command_e get_net_state_cmd(){
  command_e ret;
  if (pdTRUE != xSemaphoreTake(network_state_mutex, NET_MUTEX_WAIT)) {
      ESP_LOGE(TAG, "FAILED TO TAKE net mutext!");
      ASSERT(0);
  }
  ret = net_context.curr_cmd;
  xSemaphoreGive(network_state_mutex);

  return ret;
}

static void set_net_state_token(){
  static int token;
  if (pdTRUE != xSemaphoreTake(network_state_mutex, NET_MUTEX_WAIT)) {
      ESP_LOGE(TAG, "FAILED TO TAKE net mutext!");
      ASSERT(0);
  }
  net_context.token = token;
  xSemaphoreGive(network_state_mutex);
}

int get_net_state_token(){
  int ret;
  if (pdTRUE != xSemaphoreTake(network_state_mutex, NET_MUTEX_WAIT)) {
      ESP_LOGE(TAG, "FAILED TO TAKE net mutext!");
      ASSERT(0);
  }
  ret = net_context.token;
  xSemaphoreGive(network_state_mutex);

  return ret;
}

void driver_b(void * arg){
  network_state_init();

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
      
      send_cmd(str, len, dummy_callbuck, CFUN);
      
/*
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
  */   
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



