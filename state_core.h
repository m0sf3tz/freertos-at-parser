#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "stdbool.h"

/*********************************************************
*                     TYPEDEFS
**********************************************************/
typedef uint32_t state_event_t; // Which event
typedef uint32_t state_t;       // Which state in a state machine

// Individual state functions in a state machine
typedef void (*func_ptr)(void);

// Defines the individual states, and if those states are reinterant,
// for example, if a state has loop_timer set to 1 second, after 1 second
// of not getting an event, it will run, and so forth.
typedef struct {
    // Function pointer to the state
    func_ptr state_function_pointer;

    // If non-zero, how many times a second to run a state
    uint32_t loop_timer;

} state_array_s;

// Init function, used to set up a state machine
typedef struct {

    // This is the function that calculates the next state, based on input
    void (*next_state)(state_t*, state_event_t);

    // This function sends events to the state machine (internal use only!)
    void (*send_event)(state_event_t);

    // This function reads events to this state machine
    state_event_t (*get_event)(uint32_t);

    // Translates a state_e item to a state_array_s object
    state_array_s (*translator)(state_t);

    // Translates a event to a string (just for debug)
    char* (*event_print)(state_event_t);

    // Initial state of the state machine
    state_t starting_state;

    // For debug, name of the state
    char* state_name_string;

    // This function filters events so a state machine
    // can decide what events to react too
    bool (*filter_event)(state_event_t);

} state_init_s;

/**********************************************************
*                   GLOBAL FUNCTIONS
**********************************************************/
void state_post_event(state_event_t event);
void state_core_spawner();
void start_new_state_machine(state_init_s* state_ptr);

/**********************************************************
*                      GLOBALS    
*********************************************************/
extern QueueHandle_t events_net_q;

/**********************************************************
*                      DEFINES
**********************************************************/
#define INVALID_EVENT         (0xFFFFFFFF)
#define EVENT_QUEUE_MAX_DEPTH (16)
#define SATE_MUTEX_WAIT       (2500 / portTICK_PERIOD_MS)
