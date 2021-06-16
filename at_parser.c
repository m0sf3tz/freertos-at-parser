#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "uart_core.h"
#include "sim_stream.h"
#include "at_parser.h"
#include "global_defines.h"


/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char      TAG[] = "AT_PARSER";
static at_parsed_s     parsed;
static at_urc_parsed_s urc_parsed;
static uint8_t         buffer[1024];              // Internal
static uint8_t         line_found[MAX_LINE_SIZE]; // Returned to higher layers

/**********************************************************
*                                          IMPLEMENTATION *
**********************************************************/

// returns FALSE if s is not a number
// result returned in... result =)
static bool myatoi(char *src, int* result) {
  char buff[32];

  if (!result || !src){
      ESP_LOGE(TAG, "NULL INPUTS!");
			ASSERT(0);
	}
  memset(buff, 0, sizeof(buff));

  // dealing with a number in this format
  // "NUMBER"
  if (*src == '"'){
    memcpy(buff, src+1, strlen(src) - 2); // strip the final "
  } else {
    memcpy(buff, src, strlen(src));
  }

   char * s = buff;
	 int res = 0;
   int minus = *s == '-';
	 if (minus){
	 	s++;
	 }
	
   // exect to be all numbers, and NULL termination
   char *t = s;
   while (isdigit(*t)) {
   		*t++;
   } 	
   if(*t == '\0'){
    //dealing with a number
    goto number;
   } 

   // try again, pervious scan failed, checking for a hex number
   // this time 
   t = s;
   while (isxdigit(*t)) {
   		*t++;
   } 	
   if(*t != '\0'){ // final character must be a NULL!
     // final character was not NULL and also not HEX
     return false;
   }

   res = strtol (s,NULL,16);
   *result = ( minus ? -res : res );
	 return true;

number:
  while (isdigit(*s)) {
    res= res*10 + (*s++ - '0');
  }

  *result = ( minus ? -res : res );
  return true;
}

at_type_t get_type_cmd(char *s){
	if (!s){
		ESP_LOGE(TAG, "Null string!");
		ASSERT(0);	
	}
	
	if (strncmp(s, "AT+CFUN", 7)     == 0) return CFUN;
  if (strncmp(s, "AT+KUDPCFG",10)  == 0) return KUDPCFG;
  if (strncmp(s, "AT+KALTCFG",10)  == 0) return KALTCFG;
  if (strncmp(s, "AT+KBNDCFG",10)  == 0) return KBNDCFG;
  if (strncmp(s, "AT+ATI",6)       == 0) return ATI;
  if (strncmp(s, "AT+CESQ",7)      == 0) return CESQ;
  if (strncmp(s, "AT+KCNXCFG",10)  == 0) return KCNXCFG;
  if (strncmp(s, "AT+CEREG",8)     == 0) return CEREG;
  if (strncmp(s, "AT+KUDPSND",10)  == 0) return KUDPSND;
  if (strncmp(s, "AT+KUDPRCV",10)  == 0) return KUDPRCV;
  if (strncmp(s, "AT+CGACT",8)     == 0) return CGACT;
  // no match,
	return UNKNOWN_TYPE ;
}

at_type_t get_type(char *s){
	if (!s){
		ESP_LOGE(TAG, "Null string!");
		ASSERT(0);	
	}
	
	if (strcmp(s, "KUDP_RCV")   == 0) return KUDP_RCV;
	if (strcmp(s, "KUDP_NOTIF") == 0) return KUDP_NOTIF;
	if (strcmp(s, "CFUN")       == 0) return CFUN;
	if (strcmp(s, "CEREG")      == 0) return CEREG;
	if (strcmp(s, "CGREG")      == 0) return CGREG;
  if (strcmp(s, "KUDPCFG")    == 0) return KUDPCFG;
  if (strcmp(s, "KALTCFG")    == 0) return KALTCFG;
  if (strcmp(s, "KBDNCFG")    == 0) return KBNDCFG;
  if (strcmp(s, "ATI")        == 0) return ATI;
  if (strcmp(s, "CESQ")       == 0) return CESQ;
  if (strcmp(s, "KCNXCFG")    == 0) return KCNXCFG;
  if (strcmp(s, "KUDPSND")    == 0) return KUDPSND;
  if (strcmp(s, "KUDPRCV")    == 0) return KUDPRCV;
  if (strcmp(s, "KUDP_DATA")  == 0) return KUDP_DATA;
  if (strcmp(s, "CGACT")      == 0) return CGACT;
	// no match,
	return UNKNOWN_TYPE;
}


bool verify_urc_and_parse(char * str, int len){
   char type[MAX_LEN_TYPE];
   const char c[2] = ",";
   int lead = 0;
   int lag  = 0;  
   int type_int;
   char * iter = str;
   int param = 0;
   char * token = NULL;
   memset(type, 0, MAX_LEN_TYPE);

  if (!str){
    ESP_LOGE(TAG, "NULL INPUT!");
    ASSERT(0);
  }

  if (len == 0){
      ESP_LOGE(TAG, "WRONG PARAM INPUT!");
      ASSERT(0);
  }

  // must start with +
  if(str[0] != '+'){
    return false;
  }
    lag++;
    lead++;
 
    // searches for the first ":" 
    while (lead != '\0'){
     if (lead > len - 1){ // lead is zero based!
       ESP_LOGE(TAG, "Parsing failed! cant find :");
       return false;
     }
     if (str[lead] == ':'){
       break;
     }
     lead++;
   }

   if ( 0 >= (lead - lag) ) {
     ESP_LOGE(TAG, "Logic failed!");
     return false;
   }
   memcpy(type, str+lag, lead-lag);
   urc_parsed.type = get_type(type);
   
   // move it up to the first delimiter ":"
   lead = lead + 2;
   
    // seperate the individual parameters
    for(;;)
    {
        if (param==0){
          token = strtok(str+lead, ",");
        } else {
          token = strtok(NULL, c);
        }   

        if (token == NULL){
          break;
        }
        
        if (param == MAX_DELIMITERS){
          ESP_LOGE(TAG, "Exceed max params!");
          break;
        }
        strncpy(urc_parsed.param_arr[param].str, token, MAX_LEN_PARAM);
        // check to see if we can convert the parameter into an number
        int test_atoi;
        if( myatoi(urc_parsed.param_arr[param].str, &test_atoi)) {
          // dealing with a number!
          urc_parsed.param_arr[param].is_number = true;
          urc_parsed.param_arr[param].val = test_atoi;
        }	else {
          urc_parsed.param_arr[param].is_number = false;
        }
        param++;
    }
return true;
}

// debug
void at_print_lines(uint8_t* line, int len, uint8_t* rest){
  static int current_response_lines = 0;
  static char buff[MAX_LINES_AT][MAX_LINE_SIZE];

  if (len == 0) return;

  char fakebuf[100];
  memset(fakebuf, 0, 100);
  memcpy(fakebuf, line, len);
  printf("---->new line (len = %d), :%s \n", len, fakebuf);
}


// checks if is a quary or cmd
bool check_for_type(char *str,int len, parser_mode_e mode){
   if (!str || len == 0){
    ESP_LOGE(TAG, "NULL INPUT!");
    ASSERT(0);
  } 
  

  int i = 0;
  if(mode == PARSER_CMD_MODE){
    char * iter = str;
    for(; i < len; i++){
      if (*iter == '='){
        break;
      }
    }
    // make sure that '=' is not the last character
    if (*iter != len){
      return true; 
    }
    return false;
  } else {
    char * iter = str;
    for(; i < len; i++){
      if (*iter == '?'){
        break;
      }
    }
    // make sure that '?' is not the last character
    if (*iter != len){
      return true; 
    }
    return false;
  }
}

// Takes a line (AT+CFUN=1), and breaks it down into chunks
int at_line_explode (char * str, const int len, int line) {
  char type[MAX_LEN_TYPE];
  const char c[2] = ",";
  int lead = 0;
  int lag  = 0;  
  int curret_line = 0;
  int type_int;
  char * iter = str;
  discovered_form extended_at;
  int param = 0;
  char * token = NULL;

  memset(type, 0, sizeof(type));
  
  if (!str){
    ESP_LOGE(TAG, "NULL INPUT!");
    ASSERT(0);
  }

  if (len == 0){
      ESP_LOGE(TAG, "WRONG PARAM INPUT!");
      ASSERT(0);
  }

  if (0 == strncmp (str, "AT+", strlen("AT+"))){
      // if here, not an CMD (read/write/execute)
      // can be either URC or a AT cmd response
      if (line != 0){
        ESP_LOGE(TAG, "Malformed input stream!");
        return -1;  
      }
  } else {
   if (line != 0){
      goto find_first_respond;
    } else{
       ESP_LOGE(TAG, "Malformed input! (%d) %s", line, str);
       return -1;  
    }    
  }

  // strlen does not return size including null, but we need to check against null
  // hence "i <= len"
  for(int i = 0; i <= len; i++){
    switch (*iter){
      case('?'):
        ESP_LOGI(TAG, "Discovered a read");
        extended_at = READ_CMD;
        goto exit;
      case('='):
        if(*iter+1 == '?'){
          extended_at = TEST_CMD;
          goto exit;
        }
        ESP_LOGI(TAG, "Discovered a write");
        iter++;
        extended_at = WRITE_CMD;
        goto exit;
      case('\0'):
        ESP_LOGI(TAG, "Discovered a exec");
        extended_at = EXEC_CMD;
        goto exit;
      default:
       iter++;
    }
  }
  
  // Error - failed to find type
  ESP_LOGE(TAG, "Failed to find extended type!");
  return 1;

exit:
 parsed.form = extended_at;
 parsed.type = get_type_cmd(str);

 if (extended_at == WRITE_CMD){
  // move str up to CMD=1,2,3
  //            (here)  ^  
  str = iter;
  goto seperate_args;
 }
 // nothing left to do - no further args
 return 0;


// we would get here if we read, for example,
// +CEREG: 1
find_first_respond: 
  // Cant parse termination 
  if (str[lead] != '+'){
      ESP_LOGE(TAG, "can't handle, leading not +");
      return -1; 
  }
  lag++;
  lead++;
 
  // searches for the first ":" 
  while (lead != '\0'){
    if (lead > len - 1){ // lead is zero based!
      ESP_LOGE(TAG, "Parsing failed! cant find :");
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
  if (parsed.type != get_type(type)){
    ESP_LOGE(TAG, "type miss-match!");
    return -1;
  }

  // move it up to the first delimiter 2 after ":"
  lead = lead + 2;
 
seperate_args:
    // seperate the individual parameters
    for(;;)
    {
        if (param==0){
          token = strtok(str+lead, ",");
        } else {
          token = strtok(NULL, c);
        }   

        if (token == NULL){
          break;
        }
        
        if (param == MAX_DELIMITERS){
          ESP_LOGE(TAG, "Exceed max params!");
          break;
        }
        strncpy(parsed.param_arr[line][param].str, token, MAX_LEN_PARAM);
        // check to see if we can convert the parameter into an number
        int test_atoi;
        if( myatoi(parsed.param_arr[line][param].str, &test_atoi)) {
          // dealing with a number!
          parsed.param_arr[line][param].is_number = true;
          parsed.param_arr[line][param].val = test_atoi;
        }	else {
          parsed.param_arr[line][param].is_number = false;
        }
        param++;
    }
return 0;
}

void clear_at_parsed_struct(){
  memset(&parsed, 0, sizeof(at_parsed_s));
}

at_parsed_s * get_parsed_struct(){
  return &parsed;
}

at_urc_parsed_s * get_urc_parsed_struct(){
  return &urc_parsed;
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
at_modem_respond_e is_status_line(char * line, size_t len, int *cme_error){
  if (!line){
    ESP_LOGE(TAG, "NULL line!");
    ASSERT(0);
  }

  if (len < strlen("OK")){
    // too short to be a line termination, probably an error condition...
    ESP_LOGW(TAG, "line was really short!");
    return LINE_TERMINATION_INDICATION_NONE; 
  }

  if ( strncmp(line, "OK", len) == 0 )    return LINE_TERMINATION_INDICATION_OK; 
  if ( strncmp(line, "ERROR", len) == 0 ) return LINE_TERMINATION_INDICATION_ERROR; 
  if ( strncmp(line, "+CME ERROR:", strlen("+CME ERROR:")) == 0 ){
    int iter = 0;
    while(line[iter] != ':'){
      if(iter > len){
        ESP_LOGE(TAG, "did not find termination where expected!");
        ASSERT(0);
      }
      iter++;
    }

    // we need to go 2 more, number starts after:  (here)
    // +CME ERROR: 12
    //           ^ ^
    iter = iter + 2;
    if(iter > len){
       ESP_LOGE(TAG, "did not find termination where expected!");
       ASSERT(0);
    }
    
    if(!myatoi(line+iter, cme_error)){
       ESP_LOGE(TAG, "could not find CME error!");
       ASSERT(0);
    }
    return LINE_TERMINATION_INDICATION_CME_ERROR; 
  } 
  return LINE_TERMINATION_INDICATION_NONE; 
}

// Hunts for two "EOL" delimiters
//  1) '\r\n' [ for normal URC, response] (if data_mode == false)
//  2) '---EOF---Pattern---' , for TCP/UDP data pushes (if data_mode == true)
int at_parser_delimiter_hunter(const uint8_t c, parser_del_e mode){
  static char long_del[LONG_DELIMITER_LEN] = "--EOF--Pattern--";
  static char current_hunt[LONG_DELIMITER_LEN];
  static int iter = 0; 

  //used in searching for <CR><LF>
  static bool found_cr;

  // if not in data mode, hunts for '\r\n'
  if ( c == '\r' && (mode == PARSER_CMD_DEL) ){
    found_cr = true;
    return NO_DELIMITER;
  } 
  if (found_cr){
    if ( c == '\n'){
      found_cr = false;
      return NEW_LINE_DELIMITER; 
    } else {
      found_cr = false;
      return NO_DELIMITER;
    }
  }
  
  current_hunt[iter] = c;
  int rc = memcmp(long_del, current_hunt, iter + 1); //must test at least one character
  if (rc == 0) {
    iter++;
    if (iter == LONG_DELIMITER_LEN){
      iter = 0;
      memset(current_hunt, 0, sizeof(current_hunt));
      return LONG_DELIMITER_FOUND;
    } else {
      return PARTIAL_DELIMETER_SCANNING;
    }
  } else {
    iter = 0;
  }

  return NO_DELIMITER;
}


// this does not handle "zero" len responses, for example, 
// 0 : (65) A 
// 1 : (84) T 
// 2 : (43) + 
// 3 : (67) C 
// 4 : (70) F 
// 5 : (85) U 
// 6 : (78) N 
// 7 : (61) = 
// 8 : (48) 0 
// 9 : (13) <CR>
// 10: (10) <LF
//   
// 11: (13) <CR>  < (woud be parsed as a zero len item)
// 12: (10) <LF>  <
//     
// size: returns the total number of characters in the string, not including NULL
static uint8_t * at_parser_stringer_private(parser_del_e mode, int * size){
  static int iter_lead; // Reads ahead until all of end delimiter is hit 
  static int iter_lag;  // Lags behind while iter_lead hunts for EOL delimiter
  static int len;       // current length of line being parsed
  static bool found_line;
  int new_line_size = 0;
  memset(line_found, 0 , sizeof(line_found));

  for(;;){
    if(iter_lead == len){ // exhausted current buffer 
      int new_len = 0;
      uint8_t * new_buff = at_incomming_get_stream(&new_len);
      if(new_buff == NULL){
          return NULL;
      }
      
      if (found_line){ 
        len = len - iter_lead;
        found_line = false;
      }

      if (len < 0){
         ESP_LOGE(TAG, "Negative len!!");
         ASSERT(0);
      }

      if(len == 0){
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, new_buff, new_len);
        len = new_len;
        iter_lead = 0;
        iter_lag = 0;
      } else {
        if ( len + new_len > BUFF_SIZE){
          ESP_LOGE(TAG, "Exceeded buffer size... error! Resetting!");
          return NULL;
        }
        iter_lead++;
        memcpy(buffer + iter_lead, new_buff, new_len);
        len = len + new_len;
      }
    }

    // Logic error - no recovery
    if (iter_lead > MAX_LINE_SIZE){
       ESP_LOGE(TAG, "Exceeded buffer size... error! Resetting!");
       ASSERT(0);
    }

    int parse_status = at_parser_delimiter_hunter(buffer[iter_lead], mode);
    //printf("%d, len = %d, iter_lead = %d \n", parse_status, len, iter_lead);
    
    if ( NEW_LINE_DELIMITER == parse_status ){
      memcpy(line_found, buffer, iter_lead - 1); // termination = /r/n (strip /r as well) 
      memcpy(buffer, buffer + iter_lead + 1,  len - iter_lead);
      len = len - iter_lead - 1; // iter_lead "zero" based, len not 
      *size = iter_lead - 1;      
      iter_lead = 0;
      iter_lag = 0;
      found_line = true;
      at_print_lines(line_found, *size, buffer);
      return line_found;
    } else if (LONG_DELIMITER_FOUND == parse_status){
      memcpy(line_found, buffer, iter_lag);
      memcpy(buffer, buffer + iter_lead + 1,  len - iter_lead);
      len = len - iter_lead - 1; // iter_lead "zero" based, len not 
      *size = iter_lag;
      iter_lead = 0;
      iter_lag = 0;
      found_line = true;
      return line_found;
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

// like above, but does not properagte zero lenght reads
uint8_t * at_parser_stringer(parser_del_e mode, int * len){
  if (!len){
    ESP_LOGE(TAG, "len == null");
    ASSERT(0);
  }
  *len = 0;
  int count =0;
  uint8_t * ret;

  for(;;){
    ret = at_parser_stringer_private(mode, len);
    if (ret == NULL){
      return NULL;
    }

    if (*len == 0){
      if ( count != 2){
        count++;
        continue;
      } else {
        ESP_LOGE(TAG, "Failed to read non zero line!");
        return NULL;
      }
    } else {
      return ret;
    }
  }
}


void print_parsed(){
  printf("type     == (%d) \n", parsed.type);
  printf("form     == (%d) \n", parsed.form);
  printf("response == (%d) \n", parsed.response);
  printf("token    == (%d) \n", parsed.token);
#if 1
  for (int j = 0; j < MAX_LINES_AT ; j++){
      ESP_LOGI(TAG, "Printing line %d", j);
      for (int i = 0; i < MAX_DELIMITERS; i ++){
		    printf("%s %d %d \n", parsed.param_arr[j][i].str, parsed.param_arr[j][i].is_number, parsed.param_arr[j][i].val);
	    }
  }
#endif 
}

void print_parsed_urc(at_urc_parsed_s * urc){
  ESP_LOGI(TAG, "type == (%d) \n", urc->type);

  for (int i = 0 ; i < MAX_DELIMITERS; i++){
	  ESP_LOGI(TAG, "%s %d %d \n", urc->param_arr[i].str, urc->param_arr[i].is_number, urc->param_arr[i].val);
  }
}

void parser_test(){

  puts("starting test");

  int cme;
  char t1[] = "OK";
  at_modem_respond_e ret = is_status_line(t1, strlen(t1), &cme);
  if(ret != LINE_TERMINATION_INDICATION_OK){
    ASSERT(0);
  }
 
  char t2[] = "ERROR";
  ret  = is_status_line(t2, strlen(t2), &cme);
  if(ret != LINE_TERMINATION_INDICATION_ERROR){
    ASSERT(0);
  }

  char t3[] = "+CME ERROR: 500";
  ret  = is_status_line(t3, strlen(t3), &cme);
  if(ret != LINE_TERMINATION_INDICATION_CME_ERROR){
    ASSERT(0);
  }

  printf("CME = %d\n", cme);
  if(cme != 500){
    ASSERT(0);
  }

  char first_1[] = "AT+CFUN=1,2"; 
  at_line_explode (first_1, strlen(first_1), 0);
  print_parsed();
  if(parsed.form != WRITE_CMD){
    ASSERT(0);
  }
  if(parsed.type != CFUN){
    ASSERT(0);
  }
  if(parsed.param_arr[0][0].val != 1){
    ASSERT(0);
  }
  
  
  char test[] = "AT+CFUN=1,2"; 
  at_line_explode (test, strlen(test), 0);
  char test2[] = "+CFUN: 4,5"; 
  at_line_explode (test2, strlen(test2), 1);
  char test3[] = "+CFUN: 6,7"; 
  at_line_explode (test3, strlen(test3), 2);
  print_parsed();

  if(parsed.form != WRITE_CMD){
    ASSERT(0);
  }
  if(parsed.type != CFUN){
    ASSERT(0);
  }
  if(parsed.param_arr[1][0].val != 4){
    ASSERT(0);
  }
  if(parsed.param_arr[2][1].val != 7){
    ASSERT(0);
  }
 
  memset(&parsed, 0, sizeof(at_parsed_s));
  ESP_LOGI(TAG, "testing AT+CESQ");
  char CESQ_START[] = "AT+CESQ";
  char CESQ_RSP[]   = "+CESQ: 99,99,255,255,03,72";
  at_line_explode(CESQ_START, sizeof(CESQ_START), 0);
  at_line_explode(CESQ_RSP, sizeof(CESQ_RSP), 1);
  print_parsed();

  if(parsed.form != EXEC_CMD){
    ASSERT(0);
  }
  if(parsed.type != CESQ){
    ASSERT(0);
  }
  if(parsed.param_arr[1][0].val != 99){
    printf("%d val \n", parsed.param_arr[0][0].val);
    ASSERT(0);
  }
  if(parsed.param_arr[1][5].val != 72){
    ASSERT(0);
  }

  memset(&parsed, 0, sizeof(at_parsed_s));
  puts("testing AT+CFUN?");
  char CFUN_READ[]     = "AT+CFUN?";
  char CFUN_READ_RSP[] = "+CFUN: 1";
  
  at_line_explode(CFUN_READ, sizeof(CFUN_READ), 0);
  at_line_explode(CFUN_READ_RSP, sizeof(CFUN_READ_RSP), 1);
  print_parsed();
  
  if(parsed.form != READ_CMD){
    ASSERT(0);
  }
  if(parsed.type != CFUN){
    ASSERT(0);
  }
  if(parsed.param_arr[1][0].val != 1){
    printf("%d val \n", parsed.param_arr[0][0].val);
    ASSERT(0);
  }

 puts("");
 puts("TESTING URC ");
 char test_urc_string[] = "+KUDP_NOTIF: 9,2,3,4";
 bool urc_test = verify_urc_and_parse(test_urc_string, strlen(test_urc_string));
 printf("%d == type \n", urc_parsed.type);
 for (int i = 0; i < MAX_DELIMITERS; i ++){
		printf("%s %d %d \n", urc_parsed.param_arr[i].str, urc_parsed.param_arr[i].is_number, urc_parsed.param_arr[i].val);
	}

 if (urc_parsed.param_arr[0].val != 9){
  ASSERT(0);
 }
 if(urc_parsed.type != KUDP_NOTIF){
  ASSERT(0);
 }

 memset(&urc_parsed, 0, sizeof(urc_parsed));
 char test_urc_string_2[] = "+CEREG: 1234567";
 urc_test = verify_urc_and_parse(test_urc_string_2, strlen(test_urc_string_2));
 printf("%d == type \n", urc_parsed.type);
 for (int i = 0; i < MAX_DELIMITERS; i ++){
		printf("%s %d %d \n", urc_parsed.param_arr[i].str, urc_parsed.param_arr[i].is_number, urc_parsed.param_arr[i].val);
	}

 if (urc_parsed.param_arr[0].val != 1234567){
  ASSERT(0);
 }
 if(urc_parsed.type != CEREG){
  ASSERT(0);
 }
}
