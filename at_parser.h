#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define UNKNOWN_TYPE  (-1)

#define MAX_LINES_AT   (10)
#define MAX_DELIMITERS (7)
#define MAX_LEN_TYPE   (50)
#define MAX_LEN_PARAM  (35)
#define MAX_LEN_RAW    (150)

#define LINE_WAS_NOT_DATA_RELATED (0)
#define LINE_WAS_DATA_RELATED (1)
#define LINE_WAS_CONNECT (2)

#define NEW_LINE_DELIMITER         (0)  // Found an EOL
#define NO_DELIMITER               (1)  // No EOL character
#define PARTIAL_DELIMETER_SCANNING (2)  // Partial EOL [returned when partial ---EOF--Pattern detected]
#define LONG_DELIMITER_FOUND       (3)  // returned when full long (--EOF...) delimiter found

#define BUFF_SIZE           (1024) // Intermediate buffer
#define MAX_LINE_SIZE       (512) // Maximum AT parsed line (includes reads/writes)
#define MAX_QUEUED_ITEMS    (2)
#define LONG_DELIMITER_LEN  (16) // 18 == len(--EOF--Pattern--)

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef int at_type_t;
typedef int at_status_t;

typedef enum {
 LINE_TERMINATION_INDICATION_NONE,
 LINE_TERMINATION_INDICATION_OK,
 LINE_TERMINATION_INDICATION_ERROR, 
 LINE_TERMINATION_INDICATION_CME_ERROR, 
} at_modem_respond_e;

typedef enum {
  AT_PROCESSED_GOOD,
  AT_PROCESSED_TIMEOUT
} at_processed_status_e;

typedef enum {
  KUDP_RCV     = 1,
  KUDP_NOTIF,
  CFUN,
  CEREG,
  CGREG,
  KUDPCFG,
  KALTCFG,
  KBNDCFG,
  ATI,
  CESQ,
  KCNXCFG,
  KUDPSND
} command_e;

typedef enum {
  READ_CMD,
  WRITE_CMD,
  EXEC_CMD,
  TEST_CMD
} discovered_form;

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

  // set to either OK or ERROR, this is not the AT respond from modem,
  // for examlpe, if the parser_state times out reading from the UART 
  // port, this would be set accordingly
  at_processed_status_e status;

  // READ/WRITE/EXEC
  discovered_form form;

  // The number of lines the command returned
  // IE, for the KBNDCFG example above, this would
  // be set to 2
  size_t num_lines;

  // Parameters, for example on,
	// +KUDP_RCV: "54.189.156.244",...
  // the first param is "54.1...", etc
	at_param_s param_arr[MAX_LINES_AT][MAX_DELIMITERS];

  // network_parser sets a token which
  // at_parser puts into the reponse
  // to make sure both state machines 
  // are in sync 
  int token;

  // this is the AT respond FROM the modem
  // ie one of (ERROR, OK, +CME ERROR)
  at_modem_respond_e response;

}at_parsed_s;


// Holds URC which are parsed
typedef struct {
	// holds the type of the AT response
	// for example, for URC +CREG: 1,
  // type is CREG
	at_type_t type;

  // Parameters, for example on,
	// +KUDP_RCV: "54.189.156.244",...
  // the first param is "54.1...", etc
	at_param_s param_arr[MAX_DELIMITERS];
}at_urc_parsed_s;

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
uint8_t            *at_parser_stringer(parser_del_e mode, int * len);
int                 parse_at_string(char * str, int len, parser_mode_e mode, int line);
bool                check_for_type(char *str,int len, parser_mode_e mode);
at_type_t           get_type(char *s);
bool                verify_urc_and_parse(char * str, int len);
void                print_parsed();
void                print_parsed_urc(at_urc_parsed_s * urc);
at_parsed_s        *get_parsed_struct();
at_urc_parsed_s    *get_urc_parsed_struct();
at_modem_respond_e  is_status_line(char * line, size_t len, int *cme_error);
int                 at_line_explode (char * str, const int len, int line);
void                clear_at_parsed_struct();
bool                is_connect_line(char * line, size_t len);
