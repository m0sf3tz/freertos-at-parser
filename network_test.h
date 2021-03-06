#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"
#include "event_groups.h"

/*********************************************************
*                                                DEFINES *
*********************************************************/

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef enum {
  CFUN_GOOD,
  CFUN_TIMEOUT,
  CFUN_ERROR,
  CFUN_CME,
  CFUN_URC,
} network_tests_e;


/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
void network_test();

/*********************************************************
*                                                GLOBALS *   
*********************************************************/
