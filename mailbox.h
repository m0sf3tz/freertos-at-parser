#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"
#include "event_groups.h"

/*********************************************************
*                                                DEFINES *
*********************************************************/
#define MAILBOX_WAIT_TIME_NOMINAL (2500/portTICK_PERIOD_MS)
//#define EVENT_WAIT_PERIOD    (2500/portTICK_PERIOD_MS)
#define MUTEX_TIMEOUT_PERIOD (3000/portTICK_PERIOD_MS)

#define MAILBOX_WAIT_READY     (1 << 0)
#define MAILBOX_POST_READY     (1 << 0)

#define MAILBOX_WAIT_CONNECT   (1 << 1)
#define MAILBOX_POST_CONNECT   (1 << 1)

#define MAILBOX_WAIT_WRITE     (1 << 2)
#define MAILBOX_POST_WRITE     (1 << 2)

#define MAILBOX_WAIT_PROCESSED (1 << 3)
#define MAILBOX_POST_PROCESSED (1 << 3)

#define MAILBOX_WAIT_CONSUME   (1 << 4)
#define MAILBOX_POST_CONSUME   (1 << 4)

#define MAILBOX_WAIT_URC       (1 << 5)
#define MAILBOX_POST_URC       (1 << 5)

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
void create_mailbox_freertos_objects();

void get_mailbox_sem();
void put_mailbox_sem();
bool mailbox_post(EventBits_t post_bits);
bool mailbox_wait(EventBits_t wait_bits, TickType_t wait_period);

/*********************************************************
*                                                GLOBALS *   
*********************************************************/
