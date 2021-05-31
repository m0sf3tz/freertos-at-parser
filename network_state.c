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
static state_t state_airplain_mode_func();
static state_t state_detached_func();
static state_t state_attached_func();
static state_t state_idle_func();
static state_t state_write_func();

static void set_net_state_cmd(command_e cmd);
static void set_net_state_token();
static void network_state_set_status(network_status_e status);
static bool send_cmd(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum);
static bool send_write(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum);

static int verify_kcnxfg();
static int verify_cereg();
static int verify_cfun();
static int set_cfun();
static int verify_kudpcfg();
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
       { state_airplain_mode_func , 10000         },
       { state_detached_func      , 10000         },
       { state_attached_func      , portMAX_DELAY }, 
       { state_idle_func          , portMAX_DELAY }, 
       { state_write_func         , portMAX_DELAY }, 
};


/*********************************************************
*                                        STATE FUNCTIONS *
*********************************************************/
int dummy_callback(){
  puts("dumy callback");
  print_parsed();
}

static state_t state_airplain_mode_func() {
  ESP_LOGI(TAG, "Entering airplaine mode state!");
 /* 
  memcpy(misc_buff, "AT+CFUN?\r\n", strlen("AT+CFUN?\r\n"));
  int len = strlen(misc_buff);
 
  // verify we are detatched 
  // If we are, leave state function and wait for kick into attached
  if (1 == send_cmd(misc_buff, len, verify_cfun, CFUN)){
    ESP_LOGI(TAG, "Radio is on, forcing next state!");
    return network_attaching_state;
  }
  vTaskDelay(1000/portTICK_PERIOD_MS);
*/
  // if we get here, CFUN return 0, set it on!
  ESP_LOGE(TAG, "Forcing CFUN=1");
  
  memcpy(misc_buff, "AT+CFUN=1\r\n", strlen("AT+CFUN=1\r\n"));
  int len = strlen(misc_buff);
  send_cmd(misc_buff, len, set_cfun, CFUN);

  vTaskDelay(1000/portTICK_PERIOD_MS);

  return network_attaching_state;
}

static state_t state_detached_func() {
  ESP_LOGI(TAG, "\n*************Entering detached state!***********\n");

  // At turn on, we will have URC + CEREG? asking the same thing,
  // are we connected? have a small cool down period before we start polling 
  if (xTaskGetTickCount() > 5000/portTICK_PERIOD_MS){
    memcpy(misc_buff, "AT+CEREG?\r\n", strlen("AT+CEREG?\r\n"));
    int len = strlen(misc_buff);
 
    // verify we are detatched 
    // If we are, leave state function and wait for kick into attached
    if (1 == send_cmd(misc_buff, len, verify_cereg, CEREG)) return network_attached_state;
  }
  return NULL_STATE;
}

static state_t state_attached_func() {
  ESP_LOGI(TAG, "\n**********Entering attached state!***********\n");

  create_kcnxcfg_cmd(misc_buff, MISC_BUFF_SIZE);
  send_cmd(misc_buff, strlen(misc_buff), verify_kcnxfg, KCNXCFG);

  memset(misc_buff, 0, MISC_BUFF_SIZE);
  memcpy(misc_buff, "AT+KUDPCFG=1,0\r\n", strlen("AT+KUDPCFG=1,0\r\n"));
  int len = strlen(misc_buff);
  send_cmd(misc_buff, len, verify_kudpcfg, KUDPCFG);

  return network_idle_state;
}

static state_t state_idle_func() {
  ESP_LOGI(TAG, "\n(net)-------------->Entering idle state!\n");

  return NULL_STATE;
}

static state_t state_write_func() {
  ESP_LOGI(TAG, "\n(net)-------------->Entering write state!\n");

  create_kudpsend_cmd(misc_buff, MISC_BUFF_SIZE, "192.168.0.188", 33, 100);
  send_write(misc_buff, strlen(misc_buff), dummy_callback, KUDPSND);

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

  if (*curr_state == network_idle_state) {
      switch(event){
        case (NETWORK_WRITE_REQUEST):
            ESP_LOGI(TAG, "writting!");
            *curr_state = network_write_state;
            break;
      }
  }

}

static char* event_print_func(state_event_t event) {
    return NULL;
}

static bool event_filter_func(state_event_t event) {
  switch(event){
    case(NETWORK_ATTACHED):      return true; 
    case(NETWORK_DETACHED):      return true; 
    case(NETWORK_WRITE_REQUEST): return true; 
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
        .starting_state    = network_airplaine_mode_state,
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

static int verify_kcnxfg(){
  print_parsed();
  
  at_parsed_s *parsed = get_parsed_struct();
  if(parsed->type != KCNXCFG){
    ESP_LOGE(TAG, "CMD type not kcnxfg -> (%d)", parsed->type);
    ASSERT(0);
  }
  if(parsed->form != WRITE_CMD ){
    ESP_LOGE(TAG, "form type not write -> (%d)", parsed->form);
    ASSERT(0);
  }
}

static int verify_cereg(){
  print_parsed();

  at_parsed_s *parsed = get_parsed_struct();
  if(parsed->type != CEREG){
    ESP_LOGE(TAG, "CMD type not CEREG -> (%d)", parsed->type);
    ASSERT(0);
  }
  if(parsed->form != READ_CMD ){
    ESP_LOGE(TAG, "form type not read -> (%d)", parsed->form);
    ASSERT(0);
  }

  if (parsed->param_arr[CEREG_STATUS_LINE][CEREG_STATUS_INDEX].val == CEREG_CONNECTED){
    ESP_LOGI(TAG, "Registered! - posting!");
    state_post_event(NETWORK_ATTACHED);
    return 1;
  }  
}


static int verify_cfun(){
  print_parsed();

  at_parsed_s *parsed = get_parsed_struct();
  if(parsed->type != CFUN){
    ESP_LOGE(TAG, "CMD type not CFUN -> (%d)", parsed->type);
    ASSERT(0);
  }

  if(parsed->form != READ_CMD ){
    ESP_LOGE(TAG, "form type not read -> (%d)", parsed->form);
    ASSERT(0);
  }

  if(parsed->response != LINE_TERMINATION_INDICATION_OK){
    ESP_LOGE(TAG, "response type not OK -> (%d)", parsed->response);
    return -1;
  }

  if (parsed->param_arr[CFUN_STATUS_LINE][CFUN_STATUS_INDEX].val == CFUN_RADIO_ON){
    ESP_LOGI(TAG, "Radio on!!");
    return 1;
  }  
}

static int set_cfun(){
  print_parsed();

  at_parsed_s *parsed = get_parsed_struct();
  if(parsed->type != CFUN){
    ESP_LOGE(TAG, "CMD type not CFUN -> (%d)", parsed->type);
    ASSERT(0);
  }

  if(parsed->form != WRITE_CMD ){
    ESP_LOGE(TAG, "form type not write -> (%d)", parsed->form);
    ASSERT(0);
  }

  if(parsed->response != LINE_TERMINATION_INDICATION_OK){
    ESP_LOGE(TAG, "response type not OK -> (%d)", parsed->response);
    return -1;
  }

  if (parsed->param_arr[CFUN_SET_STATUS_LINE][CFUN_STATUS_INDEX].val == CFUN_RADIO_ON){
    ESP_LOGI(TAG, "Radio on!!");
    return 1;
  }  
}

static int verify_kudpcfg(){
  print_parsed();

  at_parsed_s *parsed = get_parsed_struct();
  if(parsed->type != KUDPCFG){
    ESP_LOGE(TAG, "CMD type not KUDPCFG -> (%d)", parsed->type);
    ASSERT(0);
  }

  if(parsed->form != WRITE_CMD ){
    ESP_LOGE(TAG, "form type not write -> (%d)", parsed->form);
    ASSERT(0);
  }

  if(parsed->response != LINE_TERMINATION_INDICATION_OK){
    ESP_LOGE(TAG, "response type not OK -> (%d)", parsed->response);
    return -1;
  }
/*
  if (parsed->param_arr[CFUN_SET_STATUS_LINE][CFUN_STATUS_INDEX].val == CFUN_RADIO_ON){
    ESP_LOGI(TAG, "Radio on!!");
    return 1;
  }*/  
  return 1;
}


static bool send_cmd(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum){
  ASSERT(cmd);
  int ret;
  at_parsed_s * parsed_p = get_parsed_struct();
  
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
  if (get_net_state_token() != parsed_p->token){
    ESP_LOGE(TAG, "TOKEN NOT CORRECT!");
    ASSERT(0);
  }

  // verify status 
  if (parsed_p->status != AT_PROCESSED_GOOD){
    // unstuck parser_state
     mailbox_post(MAILBOX_POST_CONSUME);

    ESP_LOGE(TAG, "Error! status = %d", parsed_p->status);
    //TODO: handle!
    goto fail;
  }

  ESP_LOGI(TAG, "Calling CALLBACK for send_cmd!");
  ret = clb();

  if(!mailbox_post(MAILBOX_POST_CONSUME)) goto fail;

  put_mailbox_sem();
  return ret;

fail:
  ESP_LOGE(TAG, "Failed to issue cmd!");
  put_mailbox_sem(); 
  return -1;
}

static bool send_write(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum){
  ASSERT(cmd);
  int ret;
  at_parsed_s * parsed_p = get_parsed_struct();
  
  get_mailbox_sem();

  ESP_LOGI(TAG, "Posting issue write! ");
  state_post_event(EVENT_ISSUE_WRITE);

  ESP_LOGI(TAG, "waiting for ready from PARSER_STARTE!");
  if (!mailbox_wait(MAILBOX_WAIT_READY)) goto fail;
  set_net_state_cmd(cmd_enum);
  set_net_state_token();

  ESP_LOGI(TAG, "ISSUE CMD-> %s", cmd);
  if(at_command_issue_hal(cmd, len) == -1) goto fail;
 

  if(!mailbox_wait(MAILBOX_WAIT_CONNECT)) goto fail;
  at_command_issue_hal("Suzie my house is not rent free --EOF--Pattern--", 
      strlen("Suzie my house is not rent free --EOF--Pattern--"));

  if(!mailbox_post(MAILBOX_POST_WRITE)) goto fail;

  ESP_LOGI(TAG, "waiting for processed CMD from PARSER_STATE");
  if(!mailbox_wait(MAILBOX_WAIT_PROCESSED)) goto fail;

  // verify status 
  if (parsed_p->status != AT_PROCESSED_GOOD){
    // unstuck parser_state
     mailbox_post(MAILBOX_POST_CONSUME);

    ESP_LOGE(TAG, "Error! status = %d", parsed_p->status);
    //TODO: handle!
    goto fail;
  }

  ESP_LOGI(TAG, "Calling CALLBACK for send_cmd!");
  ret = clb();

  put_mailbox_sem();
  return ret;

fail:
  ESP_LOGE(TAG, "Failed to issue cmd!");
  put_mailbox_sem(); 
  return -1;
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

static void network_state_set_status(network_status_e status){
  if (pdTRUE != xSemaphoreTake(network_state_mutex, NET_MUTEX_WAIT)) {
    ESP_LOGE(TAG, "Failed to take net state mutext!");
    ASSERT(0);
  }
  net_context.net_state = status;

  xSemaphoreGive(network_state_mutex);
}



void driver_b(void * arg){
  network_state_init();

  puts("Test");
  int len;
  char str[300]; 
  memset(str, 0, 100);
  int r = 0;

  vTaskDelay(40000/portTICK_PERIOD_MS);

  puts("hi sam!");

  state_post_event(NETWORK_WRITE_REQUEST); 
  
  vTaskDelay(1000000);
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



