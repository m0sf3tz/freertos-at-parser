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
static  at_parsed_s parsed;

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

int parse_at_string(char *str, size_t len) {
  char type[MAX_LEN_TYPE];
  const char c[2] = ",";
  int lead = 0;
  int lag  = 0;  
  memset(type, 0, sizeof(type));
  
  if (!str){
    ESP_LOGE(TAG, "NULL INPUT!");
    ASSERT(0);
  }

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
}
