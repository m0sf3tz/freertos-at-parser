#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include <errno.h>

#include "network_test.h"
#include "sim_stream.h"
#include "global_defines.h"
#include "at_parser.h"
#include "state_core.h"
#include "mailbox.h"

/**********************************************************
*                                    FORWARD DECLERATIONS *
**********************************************************/
static state_t state_idle_func();
static state_t state_handle_cmd_func();
static state_t state_handle_write_func();

static QueueSetHandle_t get_response;
static QueueSetHandle_t puts_response;
static QueueSetHandle_t uart_rx_q; 

static delay_command_s * curr_cmd;
static int curr_unit;
static int start_time;

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static const char      TAG[] = "SIM_STREAM";

static char at_cfun_good_0[] = "AT+CFUN?\r\n";
static char at_cfun_good_1[] = "+CFUN: 1\r\n";
static char at_cfun_good_2[] = "+CFUN: 2\r\n";
static char at_cfun_good_3[] = "\r\n";
static char at_cfun_good_4[] = "OK\r\n";

static char at_cfun_error_0[] = "AT+CFUN=0\r\n";
static char at_cfun_error_1[] = "ERROR\r\n";

static char at_cfun_cme_0[] = "AT+CFUN=0\r\n";
static char at_cfun_cme_1[] = "+CME ERROR: 16\r\n";

static char at_cfun_urc_0[] = "+CEREG: 1\r\n";
static char at_cfun_urc_1[] = "AT+CFUN?\r\n";
static char at_cfun_urc_2[] = "+CFUN: 1\r\n";
static char at_cfun_urc_3[] = "\r\n";
static char at_cfun_urc_4[] = "OK\r\n";

static char at_kupdsnd_echo[] = "AT+KUDPSND=\r\n";
static char at_kupdsnd_connect[] = "CONNECT\r\n";

static delay_command_s at_cfun_good = {
  .units[0] = {10, at_cfun_good_0},
  .units[1] = {20, at_cfun_good_1},
  .units[2] = {30, at_cfun_good_2},
  .units[3] = {40, at_cfun_good_3},
  .units[4] = {50, at_cfun_good_4},
};

static delay_command_s at_cfun_timeout = {
  .units[0] = {10, at_cfun_good_0},
  .units[1] = {20, at_cfun_good_1},
  .units[2] = {30, at_cfun_good_2},
  .units[3] = {6000, at_cfun_good_3},
  .units[4] = {50, at_cfun_good_4},
};

static delay_command_s at_cfun_error = {
  .units[0] = {10, at_cfun_error_0},
  .units[1] = {20, at_cfun_error_1},
};

static delay_command_s at_cfun_cme = {
  .units[0] = {10, at_cfun_cme_0},
  .units[1] = {20, at_cfun_cme_1},
};

static delay_command_s at_cfun_urc = {
  .units[0] = {10, at_cfun_urc_0},
  .units[1] = {20, at_cfun_urc_1},
  .units[2] = {30, at_cfun_urc_2},
  .units[3] = {40, at_cfun_urc_3},
  .units[4] = {50, at_cfun_urc_4}
};


// Translation Table
static state_array_s sim_translation_table[sim_state_len] = {
       { state_idle_func          ,  portMAX_DELAY },
       { state_handle_cmd_func    ,  portMAX_DELAY }, 
       { state_handle_write_func  ,  portMAX_DELAY }, 
};

/**********************************************************
*                                         STATE FUNCTIONS *
**********************************************************/
static state_t state_idle_func() {
  return NULL_STATE;
}


static state_t state_handle_cmd_func () {
   ESP_LOGI(TAG, "entering cmd sim");
   mailbox_post(MAILBOX_POST_SIM);

   int curr_unit = 0;
   int dont_care;
   do{
    xQueueReceive(get_response, &dont_care, portMAX_DELAY);
    if(dont_care == 2){
      return sim_idle_state; 
    }
    int curr_time = xTaskGetTickCount() - start_time;
    curr_time     = curr_time / portTICK_PERIOD_MS;  

    delay_unit_s  unit = curr_cmd->units[curr_unit];

    if(curr_time < unit.delay){
     uint8_t * ptr = NULL;
     xQueueSend(puts_response, &ptr, 0);
     continue;
    }

    xQueueSend(puts_response, &unit.info, 0);
    curr_unit++;
    if ( curr_cmd->units[curr_unit].info == NULL)
    {
      return sim_idle_state; 
    }

   }while(true);
}


static state_t state_handle_write_func () {
   ESP_LOGI(TAG, "entering write sim");
   mailbox_post(MAILBOX_POST_SIM);

   int magic;
   xQueueReceive(uart_rx_q, &magic, portMAX_DELAY);
   if(magic == 2){
    puts("AT+KUDPSND rxed");
    xQueueSend(puts_response, &at_kupdsnd_echo, 0);
    xQueueSend(puts_response, &at_kupdsnd_connect, 0);
  }
  
   return sim_idle_state;
}

/**********************************************************
*                                               FUNCTIONS *
**********************************************************/

void start_sim_write(){
  state_post_event(EVENT_SIMULATE_WRITE);
  if(!mailbox_wait(MAILBOX_WAIT_SIM,  MAILBOX_WAIT_TIME_NOMINAL)){
    ESP_LOGI(TAG, "Failed to sync up simulator");
    ASSERT(0);
  }
}

void set_current_cmd(command_e cmd){
  curr_unit = 0;
  start_time = xTaskGetTickCount();
  state_post_event(EVENT_SIMULATE_CMD);
  if(!mailbox_wait(MAILBOX_WAIT_SIM,  MAILBOX_WAIT_TIME_NOMINAL)){
    ESP_LOGI(TAG, "Failed to sync up simulator");
    ASSERT(0);
  }

  switch(cmd){
    case(CFUN_GOOD):    curr_cmd = &at_cfun_good; return;
    case(CFUN_TIMEOUT): curr_cmd = &at_cfun_timeout; return;
    case(CFUN_ERROR):   curr_cmd = &at_cfun_error; return;
    case(CFUN_CME):     curr_cmd = &at_cfun_cme; return;
    case(CFUN_URC):     curr_cmd = &at_cfun_urc; return;
  }
}

#ifdef FAKE_INPUT_STREAM_MODE 
uint8_t* at_incomming_get_stream(int *len){
   int dont_care;
   uint8_t * ret;
   xQueueSend(get_response, &dont_care, 0);
   vTaskDelay(10);
   BaseType_t rc = xQueueReceive(puts_response, &ret, 0);
   if(rc == pdFALSE)
   {
     return NULL;
   }
   if (ret == NULL)
   {
     return NULL;
   }
   *len = strlen(ret);
   return(ret);
}

int at_command_issue_hal(char *cmd, int len){
  int magic;
  if(memcmp(cmd, "AT+KUDPSND", 10) == 0){
    puts("issued");
    magic = 2;
    xQueueSend(uart_rx_q, &magic, 0);
  }
}

#endif

static void * test_thread (void * arg){
  int len;
  set_current_cmd(CFUN);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  printf("%s \n", at_incomming_get_stream(&len));
}


static char* event_print_func(state_event_t event) {
    return NULL;
}

static void next_state_func(state_t* curr_state, state_event_t event) {
    if (*curr_state == sim_idle_state) {
        if (event == EVENT_SIMULATE_CMD){
            ESP_LOGI(TAG, "Old State: sim_idle_state, Next: sim_handle_cmd_state");
            *curr_state = sim_handle_cmd_state;
            return;
        }
        if (event == EVENT_SIMULATE_WRITE){
            ESP_LOGI(TAG, "Old State: sim_idle_state, Next: sim_handle_write_state");
            *curr_state = sim_handle_write_state;
            return;
        }
    }
}

static bool event_filter_func(state_event_t event) {
  switch(event){
    case(EVENT_SIMULATE_CMD)   : return true; 
    case(EVENT_SIMULATE_WRITE) : return true; 
    defaut                     : return false;
  }
}

static state_init_s* get_sim_state_handle() {
    static state_init_s sim_state = {
        .next_state        = next_state_func,
        .translation_table = sim_translation_table,
        .event_print       = event_print_func,
        .starting_state    = sim_idle_state,
        .state_name_string = "sim_state",
        .filter_event      = event_filter_func,
        .total_states      = sim_state_len,  
    };
    return &(sim_state);
}

void sim_stream_test(){
   get_response  =  xQueueCreate(1, sizeof(int));  
   puts_response =  xQueueCreate(10, sizeof(uintptr_t));
   uart_rx_q     =  xQueueCreate(10, sizeof(int)); 

   start_new_state_machine(get_sim_state_handle());
}

void reset_sim_state_machine(){
 int unstuck = 2;
 ESP_LOGI(TAG, "Unstucking state machine");
 xQueueSend(get_response, &unstuck, 0); 
}
