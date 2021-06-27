#pragma once
#include "state_core.h"
#include "at_parser.h"

/*********************************************************
*                                       GLOBAL FUNCTIONS *
*********************************************************/
command_e get_net_state_cmd();
int       get_net_state_token();

// need these here for unit testing
bool send_cmd(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum);
bool send_write(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum);
bool send_read(uint8_t* cmd, int len, int (*clb)(void), command_e cmd_enum);


void pop_urc_handler(command_e urc);
void set_urc_handler(command_e urc, void (*handler) (void));

int verify_cgact();
int verify_kcnxfg();
int verify_cereg();
int verify_cfun();
int set_cfun();
int verify_kudpcfg();
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
  uint32_t           timeout_in_seconds; // each tick is a 1 second
  network_status_e   net_state;  
  pdp_network_status pdp_status;         // from KCNX_IND
  int                pdp_session;        // expect this to be only == 1 (won't use 2 sessions)
  bool               udp_status;         // from KUDP_IND
  int                token;              // used to keep parser and network states in sync 
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
  NETWORK_READ_REQUEST, 

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
#define MISC_BUFF_SIZE       (270) // MUST BE GREATER THAN SIZEOF(udp_packet_s)!

// keep this short - called every time
#define MAILBOX_WAIT_TIME_URC (250/portTICK_PERIOD_MS)
