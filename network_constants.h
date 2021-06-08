#pragma once

#include "stdbool.h"
#include <string.h>
#include <stdint.h>

/*********************************************************
*                                                DEFINES *
*********************************************************/

#define REMOTE_SERVER_IP "54.70.210.97"

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/

void create_kcnxcfg_cmd  ( char * str, int size );
void create_kudpsend_cmd ( char * str, int size, char * ip, uint16_t port, size_t len);
void create_kudprcv_cmd  ( char * str, int size, size_t bytes);

/*********************************************************
*                                                GLOBALS *   
*********************************************************/
