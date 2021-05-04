#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "at_parser.h"
#include "global_defines.h"


/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "AT_PARSER";
static  at_parsed_s parsed;    // Warning, extremely large!
static uint8_t   buffer[BUFF_SIZE];
QueueSetHandle_t line_feed_q;
static new_line_t new_line; 
static uint8_t   line_found[MAX_LINE_SIZE];
/**********************************************************
*                                          IMPLEMENTATION *
**********************************************************/

// returns FALSE if s is not a number
// result returned in... result =)
static bool myatoi(char *s, int* result) {
	 if (!result || !s){
			abort();
	 }

	 int res = 0;
   int minus = *s == '-';
	 if (minus){
	 	s++;
	 }
	
   // Iterate once, make sure we are dealing with a C string which is a number
   char *t = s;
   while (isdigit(*t)) {
   		*t++;
   } 	
   if(*t != '\0'){ // final character must be a NULL!
	  	return false;
   }

   while (isdigit(*s)) {
   		res= res*10 + (*s++ - '0');
   }

   *result = ( minus ? -res : res );
	 return true;
}

static at_type_t get_type(char *s){
	if (!s){
		ESP_LOGE(TAG, "Null string!");
		ASSERT(0);	
	}
	
	if (strcmp(s, "KUDP_RCV")   == 0) return KUDP_RCV;
	if (strcmp(s, "KUDP_NOTIF") == 0) return KUDP_NOTIF;

	// no match,
	return -1;
}

// this function digests lines,
// it will piece together a command  sequence
// for example after issueing
// AT+CFUN?
//
// we execpt to read
//
//  AT+CFUN? (echo)
//  +CFUN: 1
//  
//  OK
//
//  Every AT message has a repsonse, either
//  OK,
//  +CME error <N>,
//  ERROR,
//
//  we hunt for these to know that we found a full response
void at_digest_lines(uint8_t* line, size_t len){
  static int current_response_lines = 0;
  static char buff[MAX_LINES][MAX_LINE_SIZE];

  char fakebuf[100];
  memset(fakebuf, 0, 100);
  memcpy(fakebuf, line, len);
  printf("new line (len = %d), :%s \n", len, fakebuf);
  
  if (len == 0){
    return;
  }
 
  // checks to see if the modem is finished responding
  // to an AT command 
  at_status_t isl = is_status_line(line, len, 0);
  printf("todo! \n");
  abort();
#if 0
  if(isl){
    // done parsing
    parse_at_string(buff);
  } else {
   memcpy(buff[current_response_lines], fakebuf, len);
   current_response_lines++; 
  }
#endif 
}

int parse_at_string(at_response_s * raw_response, const int items) {
/*
  char type[MAX_LEN_TYPE];
  const char c[2] = ",";
  int lead = 0;
  int lag  = 0;  
  int curret_line = 0;
  memset(type, 0, sizeof(type));
  
  if (!str){
    ESP_LOGE(TAG, "NULL INPUT!");
    ASSERT(0);
  }

  while(curret_line != items){
  if (len == 0){
    ESP_LOGE(TAG, "WRONG PARAM INPUT!");
    ASSERT(0);
  }

  if (str[lead] != '+'){
      ESP_LOGE(TAG, "can't handle!");
      return -1; 
  }
  lag++;
  lead++;
 
  // searches for the first ":" 
  while (lead != '\0'){
    if (lead > len - 1){ // lead is zero based!
      ESP_LOGE(TAG, "Parsing failed!");
      return -1;
    }
    if (str[lead] == ':'){
      break;
    }
    lead++;
  }

  if ( 0 >= (lead - lag) ) {
    ESP_LOGE(TAG, "Logic failed!");
    return -1;
  }
  memcpy(type, str+lag, lead-lag);
  parsed.type = get_type(type);

  // move it up to the first delimiter
  lead = lead + 2;
  lag = lead; 
  if (lead > len - 1){ // lead is zero based!
     ESP_LOGE(TAG, "Error!");
     return -1;
  }

  // seperate the individual parameters
  int param = 0;
  char * token = strtok(str+lead, c);
  strncpy(parsed.param_arr[param].str, token, MAX_LEN_TYPE - 1);
  param++;
  for(;;)
  {
     	token = strtok(NULL, c);
  		if (token == NULL){
				break;
			}
			
			if (param == MAX_DELIMITERS){
				ESP_LOGE(TAG, "Exceed max params!");
				break;
			}
  		strncpy(parsed.param_arr[param].str, token, MAX_LEN_PARAM);
		  // check to see if we can convert the parameter into an number
			int test_atoi;
      if( myatoi(parsed.param_arr[param].str, &test_atoi)) {
			  // dealing with a number!
				parsed.param_arr[param].is_number = true;
				parsed.param_arr[param].val = test_atoi;
			}	else {
				parsed.param_arr[param].is_number = false;
			}
 		  param++;
	}

#if 1
  printf("type = %d \n", parsed.type);
	for (int i = 0; i < MAX_DELIMITERS; i ++){
		printf("%s %d %d \n", parsed.param_arr[i].str, parsed.param_arr[i].is_number, parsed.param_arr[i].val);
	}
#endif 
  return(1);
*/
}

at_parsed_s * get_response_arr(){
  return &parsed;
}

// checks for CONNECT
bool is_connect_line(char * line, size_t len){
  if (!line){
    ESP_LOGE(TAG, "NULL line!");
    ASSERT(0);
  }

  if (len < strlen("CONNECT")){
    return false; 
  }

  if ( strncmp(line, "CONNECT", len) == 0 ) return true; 
  
  return false;
}

// checks for command terminating line
// (OK, +CME ERROR: <N>, ERROR)
at_status_t is_status_line(char * line, size_t len, int *cme_error){
  if (!line){
    ESP_LOGE(TAG, "NULL line!");
    ASSERT(0);
  }

  if (len < strlen("OK")){
    // too short to be a line termination, probably an error condition...
    ESP_LOGW(TAG, "line was really short!");
    return LINE_TERMINATION_INDICATION_NONE; 
  }

  if ( strncmp(line, "OK", len) == 0 ) return LINE_TERMINATION_INDICATION_OK; 
  
  return LINE_TERMINATION_INDICATION_NONE; 
}

// Hunts for two "EOL" delimiters
//  1) '\n' [ for normal URC, response] (if data_mode == false)
//  2) '---EOF---Pattern---' , for TCP/UDP data pushes (if data_mode == true)
int at_parser_delimiter_hunter(const uint8_t c, bool data_mode){
  static char long_del[LONG_DELIMITER_LEN] = "--EOF--Pattern--";
  static char current_hunt[LONG_DELIMITER_LEN];
  static int iter = 0; 


  // if not in data mode, hunts for '\n'
  // Note, technically <CR><LF> - but I never
  // seen a CR not followed by a LF
  if ( c == '\n' && !data_mode ){
    //ESP_LOGI(TAG, "Found new-line delimiter");
    memset(current_hunt, 0, sizeof(current_hunt));
    iter = 0;
    return NEW_LINE_DELIMITER;  
  }

  current_hunt[iter] = c;
  int rc = memcmp(long_del, current_hunt, iter + 1); //must test at least one character
  if (rc == 0) {
    iter++;
    if (iter == LONG_DELIMITER_LEN){
      iter = 0;
      return LONG_DELIMITER_FOUND;
    } else {
      return PARTIAL_DELIMETER_SCANNING;
    }
  } else {
    iter = 0;
  }

  return NO_DELIMITER;
}

void at_parser_main(void * pv){
  static int iter_lead; // Reads ahead until all of end delimiter is hit 
  static int iter_lag;  // Lags behind while iter_lead hunts for EOL delimiter
  static int len;       // current length of line being parsed
  static bool found_line;
  static bool data_mode; //if set, parsing data

  ESP_LOGI(TAG, "Starting AT parser!");
  for(;;){
    if(iter_lead == len){ // exhausted current buffer 
      // two sub cases
      // -> Failed to find EOL, exhausted current buffer
      // -> Still hunting for EOL
      BaseType_t xStatus =  xQueueReceive(line_feed_q, &new_line, portMAX_DELAY);
      if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to RX line!");
        ASSERT(0);
      }
      
      if (found_line){ 
        len = len - iter_lead;
        found_line = false;
      }

      if (len < 0){
         ESP_LOGE(TAG, "Negative len!!");
         memset(buffer, 0, sizeof(buffer));
         iter_lag = 0;
         iter_lead = 0;
         continue;
      }

      if(len == 0){
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, new_line.buf, new_line.len);
        len = new_line.len;
        iter_lead = 0;
        iter_lag = 0;
      } else {
        if ( len + new_line.len > BUFF_SIZE){
          ESP_LOGE(TAG, "Exceeded buffer size... error! Resetting!");
          memset(buffer, 0, sizeof(buffer));
          iter_lag = 0;
          iter_lead = 0;
          continue;
        }
        iter_lead++;

        memcpy(buffer + iter_lead, new_line.buf, new_line.len);
        len = len + new_line.len;
      }
    }
  
    if (iter_lead > MAX_LINE_SIZE){
       ESP_LOGE(TAG, "Exceeded buffer size... error! Resetting!");
       memset(buffer, 0, sizeof(buffer));
       iter_lag = 0;
       iter_lead = 0;
       abort();
       continue;
    }

    const int parse_status = at_parser_delimiter_hunter(buffer[iter_lead], data_mode);
    //printf("%d, len = %d, iter_lead = %d \n", parse_status, len, iter_lead);
    
    if ( NEW_LINE_DELIMITER == parse_status ){
      // -1 strips the newline character
      memcpy(line_found, buffer, iter_lead); 
      at_digest_lines(line_found, iter_lead);
      memcpy(buffer, buffer + iter_lead + 1,  len - iter_lead);
      len = len - iter_lead - 1; // iter_lead "zero" based, len not 
      iter_lead = 0;
      iter_lag = 0;
      found_line = true;
    } else if (LONG_DELIMITER_FOUND == parse_status){
      memcpy(line_found, buffer, iter_lag);
      at_digest_lines(line_found, iter_lag);
      memcpy(buffer, buffer + iter_lead + 1,  len - iter_lead);
      len = len - iter_lead - 1; // iter_lead "zero" based, len not 
      iter_lead = 0;
      iter_lag = 0;
      found_line = true;
    }else if ( PARTIAL_DELIMETER_SCANNING == parse_status ) {
      if (iter_lead == len){
        continue;
      }
      iter_lead++;  
    } else {
      if (iter_lead == len){
        continue;
      }
      // neither a partial or complete delimiter found
      iter_lag++;
      iter_lead++;
    }
  }
}

void init_parser_freertos_objects(){
    line_feed_q = xQueueCreate(MAX_QUEUED_ITEMS, 500); 
    ASSERT(line_feed_q);
}

void driver (void * arg){
  ESP_LOGI(TAG, "Starting AT driver");
  new_line_t bar;
 
  bar.len = 26;
  memcpy(bar.buf, "\r\nI like--EOF--Pattern--\r\n", 26);
  xQueueSendToBack(line_feed_q, &bar, 0);
  xQueueSendToBack(line_feed_q, &bar, 0);
  vTaskDelay(100/portTICK_PERIOD_MS);
  
   vTaskDelete(NULL);
}

void main_test(){}

