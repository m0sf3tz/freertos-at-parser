#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <errno.h>

#include "global_defines.h"
#include "state_core.h"
#include "global_defines.h" 

/**********************************************************
*                                        GLOBAL VARIABLES *
**********************************************************/

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/
typedef struct node {
    struct node*  next;
    state_init_s* thread_info;
} node_t;

/**********************************************************
*                                        STATIC VARIABLES *
**********************************************************/
static const char        TAG[] = "STATE_CORE";
static QueueSetHandle_t  incoming_events_q;
static node_t*           head;
static SemaphoreHandle_t consumer_sem;

/**********************************************************
*                                               FUNCTIONS *
**********************************************************/

void add_event_consumer(state_init_s* thread_info) {
    ESP_LOGI(TAG, "Adding new state machine, name = %s", thread_info->state_name_string);

    if (pdTRUE != xSemaphoreTake(consumer_sem, SATE_MUTEX_WAIT)) {
        ESP_LOGE(TAG, "FAILED TO TAKE consumer_sem!");
        ASSERT(0);
    }

    if (head == NULL) {
        head = malloc(sizeof(node_t));
        ASSERT(head);

        head->next        = NULL;
        head->thread_info = thread_info;

        xSemaphoreGive(consumer_sem);
        return;
    }

    node_t* iter = head;
    node_t* new  = NULL;
    while (iter->next != NULL) {
        iter = iter->next;
    }
    new = malloc(sizeof(node_t));
    ASSERT(new);

    iter->next       = new;
    new->thread_info = thread_info;
    new->next        = NULL;
    xSemaphoreGive(consumer_sem);
}

// Reads from a global event queue and sends the event
// to all state machines that have registered for the event
static void event_multiplexer(void* v) {
    ESP_LOGI(TAG, "Starting event event_multiplexer");
    for (;;) {
        state_event_t event;
        BaseType_t    xStatus;

        xStatus = xQueueReceive(incoming_events_q, (void*)&event, portMAX_DELAY);
        if (xStatus != pdTRUE) {
            ESP_LOGE(TAG, "Failed to rx... can't recover..");
            ASSERT(0);
        }

        ESP_LOGI(TAG, "RXed an event! %d", event);

        // Iterate through all the registered event consumers, see if they
        // signed up for an event, and if so, send the event to them
        if (pdTRUE != xSemaphoreTake(consumer_sem, SATE_MUTEX_WAIT)) {
            ESP_LOGE(TAG, "FAILED TO TAKE consumer_sem!");
            ASSERT(0);
        }

        node_t* iter = head;
        while (iter != NULL) {
            ESP_LOGI(TAG, "Checking to see if %s is interested in event...", iter->thread_info->state_name_string);
            if (iter->thread_info->filter_event(event)) {
                ESP_LOGI(TAG, "sending event %d to %s", event, iter->thread_info->state_name_string);
                // State machine registered for event, send it!
                iter->thread_info->send_event(event);
            }
            iter = iter->next;
        }
        xSemaphoreGive(consumer_sem);
    }
}

static void state_core_init_freertos_objects() {
    //Reads and Pushes events from state-machines
    incoming_events_q = xQueueCreate(EVENT_QUEUE_MAX_DEPTH, sizeof(state_event_t)); // state-machines -> state-core
    consumer_sem      = xSemaphoreCreateMutex();

    // make sure nothing is NULL!
    ASSERT(incoming_events_q);
    ASSERT(consumer_sem);
}

void state_post_event(state_event_t event) {
    BaseType_t xStatus = xQueueSendToBack(incoming_events_q, (void*)&event, RTOS_DONT_WAIT);
    if (xStatus != pdTRUE) {
        ESP_LOGE(TAG, "Failed to enqueue to event event_multiplexer!");
        ASSERT(0);
    }
}

static void state_machine(void* arg) {
    if (!arg) {
        ESP_LOGE(TAG, "ARG = NULL!");
        ASSERT(0);
    }

    state_init_s* state_init_ptr = (state_init_s*)(arg);
    state_t       state          = state_init_ptr->starting_state;
    state_event_t new_event;

    for (;;) {
        // Get the current state information
        state_array_s state_info = state_init_ptr->translator(state);
        uint32_t      timeout    = state_info.loop_timer;
        func_ptr      state_func = state_info.state_function_pointer;

        // Run the current state;
        state_func();

        // Wait until a new event comes
        new_event = state_init_ptr->get_event(timeout);

        // Recieved an event, see if we need to change state
        state_init_ptr->next_state(&state, new_event);

        // Reset new_event
        new_event = INVALID_EVENT;
    }
}

void state_core_spawner() {
    BaseType_t rc;

    state_core_init_freertos_objects();
    rc = xTaskCreate(event_multiplexer,
                     "event_multiplexer",
                     4096,
                     NULL,
                     4,
                     NULL);

    if (rc != pdPASS) {
        ASSERT(0);
    }
}

void start_new_state_machine(state_init_s* state_ptr) {
    if (!state_ptr) {
        ESP_LOGE(TAG, "ARG==NULL!");
        ASSERT(0);
    }

    // Sanity check...
    if (state_ptr->event_print == NULL || state_ptr->get_event == NULL || state_ptr->next_state == NULL) {
        ESP_LOGE(TAG, "ERROR! state pointer was null!");
        ASSERT(0);
    }

    if (state_ptr->send_event == NULL || state_ptr->translator == NULL) {
        ESP_LOGE(TAG, "ERROR! state pointer was null!");
        ASSERT(0);
    }

    // Register new state machine with event multiplexer
    add_event_consumer(state_ptr);

    ESP_LOGI(TAG, "Starting new state %s", state_ptr->state_name_string);
    BaseType_t rc = xTaskCreate(state_machine,
                                state_ptr->state_name_string,
                                4096,
                                (void*)state_ptr,
                                4,
                                NULL);

    if (rc != pdPASS) {
        ASSERT(0);
    }
}
