#pragma once

#include "stdbool.h"
#include <string.h>
#include <stdint.h>

/*********************************************************
*                                                DEFINES *
*********************************************************/

#define CFUN_ENABLE_STR  "AT+CFUN=1\r\n"
#define REMOTE_SERVER_IP "54.70.210.97"

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/

void create_cgact_cmd    ( char * str, int size );
void create_kcnxcfg_cmd  ( char * str, int size );
void create_kudpsend_cmd ( char * str, int size, char * ip, uint16_t port, size_t len);
void create_kudprcv_cmd  ( char * str, int size, size_t bytes);
void create_cfun_en_cmd  ( char * str, int size);
/*********************************************************
*                                                GLOBALS *   
*********************************************************/
