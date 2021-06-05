#pragma once
#include "state_core.h"
#include "at_parser.h"

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
command_e get_net_state_cmd();
int       get_net_state_token();
/*********************************************************
*                                                GLOBALS *
*********************************************************/

/*********************************************************
*                                               TYPEDEFS *
*********************************************************/
typedef int net_mode_t;

typedef struct{
  int read_data;
}work_order_s;

// from KCNX_IND
typedef enum {
  disconnected,
  connected,
  failed_to_connect,
  closed,
  connecting,
} pdp_network_status;

typedef enum {
  network_state_connected
} network_status_e;

typedef struct{
  net_mode_t         mode;
  command_e          curr_cmd;
  network_status_e   net_state;  
  pdp_network_status pdp_status;  // from KCNX_IND
  int                pdp_session; // expect this to be only == 1 (won't use 2 sessions)
  bool               udp_status;  // from KUDP_IND
  int                token;       // used to keep parser and network states in sync 
}network_state_s;

typedef enum {
  network_airplaine_mode_state = 0,
  network_attaching_state,
  network_attached_state,
  network_idle_state,
  network_write_state,
  network_read_state,

  network_state_len //LEAVE AS LAST!
} network_state_e;


typedef enum {
  NETWORK_ATTACHED = NETWORK_EVENT_START,
  NETWORK_DETACHED,
  NETWORK_WRITE_REQUEST, 

  network_event_len //LEAVE AS LAST!
} network_event_e;


/*********************************************************
*                                                  ENUMS *
*********************************************************/

/*********************************************************
*                                                DEFINES *
*********************************************************/
#define NET_MUTEX_WAIT       (2500 / portTICK_PERIOD_MS)
#define PDP_INVALID          (-1)
#define MISC_BUFF_SIZE       (100)

// keep this short - called every time
#define MAILBOX_WAIT_TIME_URC (250/portTICK_PERIOD_MS)
