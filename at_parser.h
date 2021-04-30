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

#define MAX_LINES      (10)
#define MAX_DELIMITERS (5)
#define MAX_LEN_TYPE   (50)
#define MAX_LEN_PARAM  (100)
#define MAX_LEN_RAW    (150)

#define LINE_TERMINATION_INDICATION_NONE (0)
#define LINE_TERMINATION_INDICATION_OK (1)
#define LINE_TERMINATION_INDICATION_ERROR (2)
#define LINE_TERMINATION_INDICATION_CME_ERROR (3)

#define NEW_LINE_DELIMITER         (0)  // Found an EOL
#define NO_DELIMITER               (1)  // No EOL character
#define PARTIAL_DELIMETER_SCANNING (2)  // Partial EOL [returned when partial ---EOF--Pattern detected]
#define LONG_DELIMITER_FOUND       (3)  // returned when full long (--EOF...) delimiter found



/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef int at_type_t;
typedef int at_status_t;

//This stores an individual line of an AT response
typedef struct{ 
  char  str[MAX_LEN_RAW];
  size_t len;
}at_response_s; 

// IE)
//  AT+KBNDCFG?
//  +KBNDCNFG: ....
//  +KBNDCNFG: ....
// 
//  OK
//
// In the above example, each individual line is passed
// to the AT parser in a at_response_s packet through
// an at_response_t object (which is an array of at_response_s)


typedef struct {
    bool is_number;          // If the AT response is a number (ie, +CEREG: 1,2)
		int  val;                // If the AT response is a number, the numerical form of that number
		char str[MAX_LEN_PARAM]; // The string form (raw) of each response
}at_param_s;

typedef struct {
	// holds the type of the AT response
	// for example, for URC +CREG: 1,
  // type is CREG
	at_type_t type;

  // set to either OK or ERROR based on the command status.
  at_status_t status;

  // The number of lines the command returned
  // IE, for the KBNDCFG example above, this would
  // be set to 2
  size_t num_lines;

  // Parameters, for example on,
	// +KUDP_RCV: \"54.189.156.244\"
  // the first param is "54.1...", etc
	at_param_s param_arr[MAX_LINES][MAX_DELIMITERS];
}at_parsed_s;


/**********************************************************
*                                        GLOBAL FUNCTIONS *
**********************************************************/
int parse_at_string(at_response_s *raw_response, const int items);
at_parsed_s * get_response_arr();

/**********************************************************
*                                                 GLOBALS *   
**********************************************************/
