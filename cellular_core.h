#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"


/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/

/*********************************************************
*                                                GLOBALS *
*********************************************************/

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                                  ENUMS *
*********************************************************/

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define MAX_OUTSTANDING_WRITE_COMMANDS (2)
#define MAX_WRITE_COMMAND_LEN          (128)

