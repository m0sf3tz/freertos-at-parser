#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"

/*********************************************************
*                                                DEFINES *
*********************************************************/
#define EVENT_WAIT_PERIOD    (2500/portTICK_PERIOD_MS)
#define MUTEX_TIMEOUT_PERIOD (2500/portTICK_PERIOD_MS)

#define MAILBOX_WAIT_FOR_READY (1 << 0)
#define MAILBOX_POST_READY     (1 << 0)

#define MAILBOX_WAIT_FOR_POST_WORK_REQUEST (1 << 1)
#define MAILBOX_POST_WORK_REQUEST          (1 << 1)

#define MAILBOX_WAIT_FOR_WORK_DONE         (1 << 2)
#define MAILBOX_POST_WORK_DONE             (1 << 2)

#define MAILBOX_WAIT_FOR_CONSUME           (1 << 3)
#define MAILBOX_POST_CONSUME               (1 << 3)
/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/**********************************************************
*                                        GLOBAL FUNCTIONS *
**********************************************************/
bool wait_mail_box();
bool wait_mail_ring();

/**********************************************************
*                                                 GLOBALS *   
**********************************************************/
