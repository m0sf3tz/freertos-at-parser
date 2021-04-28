#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define KUDP_RCV     (1)
#define KUDP_NOTIF   (2)
#define UNKNOWN_TYPE (-1)

#define MAX_DELIMITERS (5)
#define MAX_LEN_TYPE   (50)
#define MAX_LEN_PARAM  (100)

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef int at_type_t;

typedef struct {
    bool is_number;
		int  val;
		char str[MAX_LEN_PARAM];
}at_param_s;

typedef struct {
	// holds the type of the AT response
	// for extample, for URC +CREG: 1,
  // type is CREG
	at_type_t type;

  // Paramters, for example on,
	// +KUDP_RCV: \"54.189.156.244\"
  // the first param is "54.1...", etc
	at_param_s param_arr[MAX_DELIMITERS];
}at_parsed_s;

/**********************************************************
*                                        GLOBAL FUNCTIONS *
**********************************************************/
int parse_at_string(char *s, size_t len);

/**********************************************************
*                                                 GLOBALS *   
**********************************************************/
