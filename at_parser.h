#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define KUDP_RCV     (1)
#define KUDP_NOTIF   (2)
#define CFUN         (3)
#define CEREG        (4)
#define CGREG        (5)
#define KUDPCFG      (6)
#define KALTCFG      (7)
#define BNDCFG       (8)
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

#define LINE_WAS_NOT_DATA_RELATED (0)
#define LINE_WAS_DATA_RELATED (1)
#define LINE_WAS_CONNECT (2)

#define NEW_LINE_DELIMITER         (0)  // Found an EOL
#define NO_DELIMITER               (1)  // No EOL character
#define PARTIAL_DELIMETER_SCANNING (2)  // Partial EOL [returned when partial ---EOF--Pattern detected]
#define LONG_DELIMITER_FOUND       (3)  // returned when full long (--EOF...) delimiter found

#define BUFF_SIZE          (1024) // Intermediate buffer
#define MAX_LINE_SIZE      (512) // Maximum AT parsed line (includes reads/writes)
#define MAX_QUEUED_ITEMS   (2)
#define LONG_DELIMITER_LEN (16) // 18 == len(--EOF--Pattern--)

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
	// +KUDP_RCV: "54.189.156.244",...
  // the first param is "54.1...", etc
	at_param_s param_arr[MAX_LINES][MAX_DELIMITERS];
}at_parsed_s;

// How uart_core packs responses to send to at_parser
typedef struct {                                     
     uint16_t len;
     uint8_t  buf[MAX_LINE_SIZE];
} new_line_t;

typedef enum {
  PARSER_FULL_MODE,
  PARSER_CMD_MODE,
  PARSER_QRY_MODE,
  PARSER_URC_MODE
} parser_mode_e;

typedef enum {
  PARSER_CMD_DEL,
  PARSER_DATA_DEL,
} parser_del_e;


/**********************************************************
*                                        GLOBAL FUNCTIONS *
**********************************************************/
uint8_t     * at_parser_stringer(parser_del_e mode, bool * status, int * len);
int           parse_at_string(char * str, int len, parser_mode_e mode, int line);
at_parsed_s * get_response_arr();


at_type_t get_type(char *s);
bool is_urc(at_type_t type);


/**********************************************************
*                                                 GLOBALS *   
**********************************************************/
