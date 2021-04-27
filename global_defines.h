#pragma once

/**********************************************************
*                      DEFINES
**********************************************************/
#define RTOS_DONT_WAIT      (0)

// Set this flag to one for POSIX FreeRTOS simulation
#define POSTIX_FREERTOS_SIM     

/**********************************************************
*                       HELPERS 
**********************************************************/
#define ASSERT(x)                                                       \
    do {                                                                \
        if (!(x)) {                                                     \
            ESP_LOGE(TAG, "ASSERT! error %s %u\n", __FILE__, __LINE__); \
        }                                                               \
    } while (0)

#define TRUE  (1)
#define FALSE (0)

#define ESP_LOGE( tag, format, ... ) printf(format, ##__VA_ARGS__); printf("\n")
#define ESP_LOGI( tag, format, ... ) printf(format, ##__VA_ARGS__); printf("\n")

