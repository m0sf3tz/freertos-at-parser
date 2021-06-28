#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "event_groups.h"
#include <errno.h>

#include "mailbox.h" 
#include "global_defines.h"
#include "network_state.h"
#include "at_parser.h"
#include "network_test.h"
#include "sim_stream.h"

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static const char         TAG[] = "NETWORK TEST";
static char               misc_buff[150];
static int                urc_test_val;

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define URC_TEST_VAL (23)

/**********************************************************
*                                               FUNCTIONS *
**********************************************************/

static void tets_urc_func(){
  urc_test_val = URC_TEST_VAL;
  ESP_LOGI(TAG, "Urc called");
}

static int verify_cfun_test(){
  return pdTRUE;
}

void network_test(){
  
  clear_parsed_struct();
  at_parsed_s * parsed_p = get_parsed_struct();
  parsed_p->status = AT_PROCESSED_LAST;  
#if 1 
  // Test normal condition
  set_current_cmd(CFUN_GOOD);
  create_cfun_en_cmd(misc_buff, 150);
  ESP_LOGI(TAG, "starting send cmd (good case)");

  send_cmd(misc_buff, strlen(misc_buff), verify_cfun_test, CFUN);
  if ( parsed_p->status != AT_PROCESSED_GOOD ){
    ESP_LOGI(TAG, "CFUN(good) failed!");
    ASSERT(0);
  }
  if (parsed_p->num_lines != 2){
    ESP_LOGI(TAG, "CFUN(good) failed!");
    ASSERT(0);   
  }
  if ( parsed_p->status != AT_PROCESSED_GOOD ){
    ESP_LOGI(TAG, "CFUN(good) failed!");
    ASSERT(0);
  }
  ESP_LOGI(TAG, "***Done cfun(good) test!***");
#endif 

  // Test timeout
  vTaskDelay(2000);
  clear_parsed_struct();
  parsed_p->status = AT_PROCESSED_LAST;  
  set_current_cmd(CFUN_TIMEOUT);
  send_cmd(misc_buff, strlen(misc_buff), verify_cfun_test, CFUN);
  if ( parsed_p->status != AT_PROCESSED_TIMEOUT ){
    ESP_LOGI(TAG, "CFUN(timeout) failed!");
    ASSERT(0);
  }
  reset_sim_state_machine();

  // Test error
  vTaskDelay(2000);
  clear_parsed_struct();
  parsed_p->status = AT_PROCESSED_LAST;  
  set_current_cmd(CFUN_ERROR);
  send_cmd(misc_buff, strlen(misc_buff), verify_cfun_test, CFUN);
  if ( parsed_p->status != AT_PROCESSED_GOOD ){
    ESP_LOGI(TAG, "CFUN(error) failed!");
    ASSERT(0);
  }
  if (parsed_p->response != LINE_TERMINATION_INDICATION_ERROR){
    ESP_LOGI(TAG, "CFUN(error) failed!");
    ASSERT(0);   
  }
  if (parsed_p->num_lines != 0){
    ESP_LOGI(TAG, "CFUN(error) failed!");
    ASSERT(0);   
  }
  ESP_LOGI(TAG, "***Done cfun(error) test!***");
#if 1
  // Test CME
  vTaskDelay(2000);
  parsed_p->status = AT_PROCESSED_LAST;  
  clear_parsed_struct();
  set_current_cmd(CFUN_CME);
  send_cmd(misc_buff, strlen(misc_buff), verify_cfun_test, CFUN);
  if ( parsed_p->status != AT_PROCESSED_GOOD ){
    ESP_LOGI(TAG, "CFUN(cme) failed!");
    ASSERT(0);
  }
  if (parsed_p->response != LINE_TERMINATION_INDICATION_CME_ERROR){
    ESP_LOGI(TAG, "CFUN(cme) failed!");
    ASSERT(0);   
  }
  if (parsed_p->num_lines != 0){
    ESP_LOGI(TAG, "CFUN(cme) failed!");
    ASSERT(0);   
  }
  if (parsed_p->cme != 16){
    ESP_LOGI(TAG, "CFUN(cme) failed!");
    ASSERT(0);   
  }
  
  // Test URC
  urc_test_val = 0;
  parsed_p->status = AT_PROCESSED_LAST;  
  set_urc_handler(CEREG, tets_urc_func);
  vTaskDelay(10);
  clear_parsed_struct();
  set_current_cmd(CFUN_URC);
  send_cmd(misc_buff, strlen(misc_buff), verify_cfun_test, CFUN);
  if ( parsed_p->status != AT_PROCESSED_GOOD ){
    ESP_LOGI(TAG, "CFUN(cme) failed!");
    ASSERT(0);
  }
  if (parsed_p->response != LINE_TERMINATION_INDICATION_OK){
    ESP_LOGI(TAG, "CFUN(URC) failed!");
    ASSERT(0);   
  }
  if (parsed_p->num_lines != 1){
    ESP_LOGI(TAG, "CFUN(URC) failed!");
    ASSERT(0);   
  }
  if (urc_test_val != URC_TEST_VAL){
    ESP_LOGI(TAG, "CFUN(URC) failed!");
    ASSERT(0);   
  }
  pop_urc_handler(CEREG);
#endif
}
