#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>

#include "global_defines.h"
#include "parser_state.h"
#include "state_core.h"
#include "at_parser.h"

/*********************************************************
*                                   FORWARD DECLARATIONS *
*********************************************************/

/*********************************************************
*                                       STATIC VARIABLES *
*********************************************************/
static const char        TAG[] = "URC_HANDLER";

/*********************************************************
*                                        IMPLEMENTATIONS *
*********************************************************/


