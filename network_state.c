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
#include "network_constants.h"
#include "net_adaptor.h"
#include "network_state.h"
#include "network_test.h"

/*********************************************************
*                                   FORWARD DECLARATIONS *
*********************************************************/
static state_t state_airplain_mode_func();
static state_t state_detached_func();
static state_t state_attached_func();
static state_t state_idle_func();
static state_t state_write_func();
static state_t state_read_func();

static void set_net_state_cmd(command_e cmd);
static void set_net_state_token();
static void network_state_set_status(network_status_e status);

static void handle_urc_kudp_notif();
/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char         TAG[] = "NET_STATE";
static network_state_s    net_context;
static at_urc_parsed_s    urc_parsed;
static SemaphoreHandle_t  network_state_mutex;
static char               misc_buff[MISC_BUFF_SIZE]; 
static work_order_s       work_order; 

// Translation Table
static state_array_s network_translation_table[network_state_len] = {
       { state_airplain_mode_func , 10000         },
       { state_detached_func      , 10000         },
       { state_attached_func      , portMAX_DELAY }, 
       { state_idle_func          , portMAX_DELAY }, 
       { state_write_func         , portMAX_DELAY }, 
       { state_read_func          , portMAX_DELAY }, 
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

  ESP_LOGE(TAG, "Forcing CFUN=1");
  memcpy(misc_buff, CFUN_ENABLE_STR  , strlen(CFUN_ENABLE_STR));
  int len = strlen(misc_buff);
  int rc  = send_cmd(misc_buff, len, set_cfun, CFUN);
  if (rc != pdTRUE){
    ESP_LOGE(TAG, "Failed to CFUN=1! - not much to do.... retry");
    vTaskDelay(60000/portTICK_PERIOD_MS);
    return network_airplaine_mode_state;
  }

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

  create_kudpsend_cmd(misc_buff, MISC_BUFF_SIZE, REMOTE_SERVER_IP, 3333, 100);
  send_write(misc_buff, strlen(misc_buff), dummy_callback, KUDPSND);

  return network_idle_state;
} 

static state_t state_read_func() {
  ESP_LOGI(TAG, "\n(net)-------------->Entering read state!\n");

  create_kudprcv_cmd(misc_buff, MISC_BUFF_SIZE, work_order.read_data);
  send_read(misc_buff, strlen(misc_buff), dummy_callback, KUDPRCV);

  return network_idle_state;
}

// Returns the next state
static void next_state_func(state_t* curr_state, state_event_t event) {
  if (*curr_state == network_attaching_state) {
      switch(event){
        case (NETWORK_ATTACHED):
            ESP_LOGI(TAG, "detached->attached");
            *curr_state = network_attached_state;
            return;
      }
  }

  if (*curr_state == network_attached_state) {
      switch(event){
        case (NETWORK_DETACHED):
            ESP_LOGI(TAG, "attached->detached");
            *curr_state = network_attaching_state;
            return;
      }
  }

  if (*curr_state == network_idle_state) {
      switch(event){
        case (NETWORK_WRITE_REQUEST):
            *curr_state = network_write_state;
            return;
        case(NETWORK_READ_REQUEST):
            *curr_state = network_read_state;
            return;
      }
  }

  if (*curr_state == network_write_state) {
      switch(event){
        case (NETWORK_READ_REQUEST):
            *curr_state = network_read_state;
            return;
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
    case(NETWORK_READ_REQUEST):  return true;
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

void set_urc_handler(command_e urc, void (*handler) (void)){
  ASSERT(handler);

  at_urc_parsed_s urc_handler;
  urc_register_s  urc_reg;
  
  urc_handler.cblk_reg = &urc_reg;
  urc_reg.urc = urc;
  urc_reg.clb = handler;

  xQueueSendToBack(outgoing_urc_queue, &urc_handler, portMAX_DELAY);
}

void pop_urc_handler(command_e urc){
  at_urc_parsed_s urc_handler;
  urc_register_s  urc_reg;
  memset(&urc_reg, 0 , sizeof(urc_reg));

  urc_handler.cblk_reg = &urc_reg;
  urc_reg.urc = urc;

  xQueueSendToBack(outgoing_urc_queue, &urc_handler, portMAX_DELAY);
}

void handle_cereg_urc(){
   if (urc_parsed.param_arr[0].val == 1){
     ESP_LOGI(TAG, "Registered! - posting!");
     state_post_event(NETWORK_ATTACHED);
   } else if (urc_parsed.param_arr[0].val != 1){ //TODO: wrong! (roaming!)
     ESP_LOGI(TAG, "detached! - posting!");
     state_post_event(NETWORK_DETACHED);
   }
}

static void handle_kudp_data(){
  work_order.read_data = urc_parsed.param_arr[1].val;
  ESP_LOGI(TAG, "new data (UDP), len = %d", work_order.read_data);

  state_post_event(NETWORK_READ_REQUEST); 
}

static void urc_hanlder(void * arg){
  void (*callback_arr[LEN_KNOWN_COMMANDS])(void);
  memset(callback_arr, 0, sizeof(callback_arr));

  for(;;){
    memset(&urc_parsed, 0 , sizeof(urc_parsed));
    xQueueReceive(outgoing_urc_queue, &urc_parsed, portMAX_DELAY);
    
    if(urc_parsed.cblk_reg){
      ESP_LOGI(TAG, "Adding/removing URC callback");
      if(urc_parsed.cblk_reg->clb == NULL){
        callback_arr[urc_parsed.cblk_reg->urc] = NULL;
      } else {
        callback_arr[urc_parsed.cblk_reg->urc] = urc_parsed.cblk_reg->clb;
      }
      continue;
    }

    ESP_LOGI(TAG, "Got a new URC!");
    print_parsed_urc(&urc_parsed);

    if(urc_parsed.type == UNKNOWN_TYPE){
      ESP_LOGE(TAG, "Unknown type!");
      continue;
    }

    if(callback_arr[urc_parsed.type]){
      ESP_LOGI(TAG, "URC handler installed, calling handler!");

      callback_arr[urc_parsed.type]();
      continue; 
    }

    // default handlers
    switch(urc_parsed.type){
      case(CEREG):     handle_cereg_urc();
      case(KUDP_DATA): handle_kudp_data();
    }
  }
}

int verify_cgact(){
  print_parsed();
  
  at_parsed_s *parsed = get_parsed_struct();
  if(parsed->type != CGACT){
    ESP_LOGE(TAG, "CMD type not CGACT -> (%d)", parsed->type);
    ASSERT(0);
  }

  if(parsed->form != READ_CMD ){
    ESP_LOGE(TAG, "form type not read -> (%d)", parsed->form);
    ASSERT(0);
  } 
}

int verify_kcnxfg(){
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

  if (parsed->param_arr[CEREG_STATUS_LINE][CEREG_STATUS_INDEX].val == CEREG_CONNECTED){
    ESP_LOGI(TAG, "Registered! - posting!");
    return pdTRUE;
  }

  return pdFALSE;
}

int verify_cereg(){
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


int verify_cfun(){
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

int set_cfun(){
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
    return pdFALSE;
  }

  if (parsed->param_arr[CFUN_SET_STATUS_LINE][CFUN_STATUS_INDEX].val == CFUN_RADIO_ON){
    ESP_LOGI(TAG, "Radio on!!");
    return pdTRUE;
  }
 
  // should not get here
  return pdFALSE;
}

int verify_kudpcfg(){
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

bool send_cmd(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum){
  ASSERT(cmd);
  int ret;
  at_parsed_s * parsed_p = get_parsed_struct();
  
  get_mailbox_sem();
  set_net_state_cmd(cmd_enum);

  ESP_LOGI(TAG, "Posting issue cmd!");
  state_post_event(EVENT_ISSUE_CMD);

  ESP_LOGI(TAG, "waiting for ready from parser state");
  if (!mailbox_wait(MAILBOX_WAIT_READY, MAILBOX_WAIT_TIME_NOMINAL)){
    ESP_LOGE(TAG, "Parser sate not ready!");
    goto fail;
  }
  set_net_state_token();

  ESP_LOGI(TAG, "ISSUE CMD-> %s", cmd);
  if(at_command_issue_hal(cmd, len) == -1){
    ESP_LOGE(TAG, "Failed to write command to uart!");
    goto fail;
  }
  
  ESP_LOGI(TAG, "waiting for processed CMD from PARSER_STATE");
  if(!mailbox_wait(MAILBOX_WAIT_PROCESSED, MAILBOX_WAIT_TIME_NOMINAL)){
    ESP_LOGE(TAG, "Failed to get processed command!");
    goto fail;
  }

  // verify token
  ESP_LOGI(TAG, "Verying token");
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

bool send_write(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum){
  ASSERT(cmd);
  int ret;
  at_parsed_s * parsed_p = get_parsed_struct();

  set_urc_handler(KUDP_NOTIF, handle_urc_kudp_notif);

  get_mailbox_sem();

  ESP_LOGI(TAG, "Posting issue write! ");
  state_post_event(EVENT_ISSUE_WRITE);

  ESP_LOGI(TAG, "waiting for ready from PARSER_STARTE!");
  if (!mailbox_wait(MAILBOX_WAIT_READY, MAILBOX_WAIT_TIME_NOMINAL)) goto fail;
  set_net_state_cmd(cmd_enum);
  set_net_state_token();

  ESP_LOGI(TAG, "ISSUE CMD-> %s", cmd);
  if(at_command_issue_hal(cmd, len) == -1) goto fail;

  if(!mailbox_wait(MAILBOX_WAIT_CONNECT, MAILBOX_WAIT_TIME_NOMINAL)) goto fail;


  BaseType_t xStatus = xQueueReceive(outgoing_udp_q, misc_buff, RTOS_DONT_WAIT);
  if (xStatus != pdTRUE){
     ESP_LOGE(TAG, "Failed to RX on event send data!");
     ASSERT(0);
  }

  // save some space, reuse misc buff
  udp_packet_s * pudp = (udp_packet_s*)misc_buff;
  at_command_issue_hal(pudp->data, pudp->len);

  if(!mailbox_post(MAILBOX_POST_WRITE)) goto fail;

  ESP_LOGI(TAG, "waiting for processed CMD from PARSER_STATE");
  if(!mailbox_wait(MAILBOX_WAIT_PROCESSED, MAILBOX_WAIT_TIME_NOMINAL)) goto fail;

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

  if(mailbox_wait(MAILBOX_WAIT_URC, MAILBOX_WAIT_TIME_URC))
  {
    // we got a URC... handle it
    puts("got urc!");
    mailbox_post(MAILBOX_POST_CONSUME);
  }

  pop_urc_handler(KUDP_NOTIF);
  put_mailbox_sem();
  return ret;

fail:
  ESP_LOGE(TAG, "Failed to issue cmd!");
  put_mailbox_sem();
  return -1;
}

bool send_read(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum){
  ASSERT(cmd);
  int ret;
  at_parsed_s * parsed_p = get_parsed_struct();

  get_mailbox_sem();

  ESP_LOGI(TAG, "Posting issue read! ");
  state_post_event(EVENT_ISSUE_READ);

  ESP_LOGI(TAG, "waiting for ready from PARSER_STARTE!");
  if (!mailbox_wait(MAILBOX_WAIT_READY, MAILBOX_WAIT_TIME_NOMINAL)) goto fail;
  set_net_state_cmd(cmd_enum);
  set_net_state_token();

  ESP_LOGI(TAG, "ISSUE CMD-> %s", cmd);
  if(at_command_issue_hal(cmd, len) == -1) goto fail;
 
  if(!mailbox_wait(MAILBOX_WAIT_CONNECT, MAILBOX_WAIT_TIME_NOMINAL)) goto fail;

  if(!mailbox_wait(MAILBOX_WAIT_PROCESSED, MAILBOX_WAIT_TIME_NOMINAL)) goto fail;

  // verify status 
  if (parsed_p->status != AT_PROCESSED_GOOD){
    // unstuck parser_state
     mailbox_post(MAILBOX_POST_CONSUME);

    ESP_LOGE(TAG, "Error! status = %d", parsed_p->status);
    //TODO: handle!
    goto fail;
  }

  mailbox_post(MAILBOX_POST_CONSUME);

  put_mailbox_sem();
  return ret;

fail:
  ESP_LOGE(TAG, "Failed to issue cmd!");
  put_mailbox_sem();
  return -1;
}


static void handle_urc_kudp_notif()
{
  mailbox_post(MAILBOX_POST_URC);
  puts("callback called!");
  mailbox_wait(MAILBOX_POST_CONSUME, MAILBOX_WAIT_TIME_URC);
}


static void network_state_init_freertos_objects() {
    network_state_mutex = xSemaphoreCreateMutex();

    // make sure we init all the rtos objects
    ASSERT(network_state_mutex);
}

static void network_state_init(){

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

static void network_state_set_timeout(uint32_t timeout_ticks){
  if (pdTRUE != xSemaphoreTake(network_state_mutex, NET_MUTEX_WAIT)) {
    ESP_LOGE(TAG, "Failed to take net state mutext!");
    ASSERT(0);
  }
  net_context.timeout_in_seconds = timeout_ticks;

  xSemaphoreGive(network_state_mutex);
}


void driver_b(void * arg){

  puts("Test");
  int len;
  char str[300]; 
  memset(str, 0, 100);
  int r = 0;

  vTaskDelay(40000/portTICK_PERIOD_MS);

  puts("hi sam!");
  
  udp_packet_s test;
  memset(&test, 0, sizeof(udp_packet_s));
a:
  sprintf(test.data, "wtf lol??\n--EOF--Pattern--");
  test.len = strlen(test.data);
  enqueue_udp_write(&test);
  
  vTaskDelay(5000/portTICK_PERIOD_MS);
  goto a;

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

void network_test_thread(void * arg){
  for(;;){
    network_test();
    vTaskDelay(10000000);  
  }
}

void network_driver(){
  network_state_init_freertos_objects();
#ifdef FAKE_INPUT_STREAM_MODE
  sim_stream_test();
  xTaskCreate(network_test_thread, "", 1024, "", 5, NULL); 
  xTaskCreate(urc_hanlder, "", 1024, "", 5, NULL); 
#else
  network_state_init();
  xTaskCreate(urc_hanlder, "", 1024, "", 5, NULL); 
  xTaskCreate(driver_b, "", 1024, "", 5, NULL); 
#endif
}
