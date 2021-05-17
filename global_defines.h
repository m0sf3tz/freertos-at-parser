#pragma once

/**********************************************************
*                                                 DEFINES *
**********************************************************/
#define RTOS_DONT_WAIT      (0)

// Set this flag to one for POSIX FreeRTOS simulation
#define POSIX_FREERTOS_SIM     

#define PARSER_CORE_EVENT_START (100)
#define NETWORK_EVENT_START     (200)

/**********************************************************
*                                                 HELPERS *
**********************************************************/

#ifdef POSIX_FREERTOS_SIM
 #define ASSERT(x)                                                       \
     do {                                                                \
         if (!(x)) {                                                     \
             ESP_LOGE(TAG, "ASSERT! error %s %u\n", __FILE__, __LINE__); \
            abort();                                                     \
         }                                                               \
     } while (0)
#else 
 #define ASSERT(x)                                                       \
     do {                                                                \
         if (!(x)) {                                                     \
             ESP_LOGE(TAG, "ASSERT! error %s %u\n", __FILE__, __LINE__); \
         }                                                               \
     } while (0) error lol fix it 
#endif 

#define TRUE  (1)
#define FALSE (0)

#ifdef POSIX_FREERTOS_SIM
 #define ESP_LOGE( tag, format, ... ) printf(format, ##__VA_ARGS__); printf("\n")
 #define ESP_LOGI( tag, format, ... ) printf(format, ##__VA_ARGS__); printf("\n")
 #define ESP_LOGW( tag, format, ... ) printf(format, ##__VA_ARGS__); printf("\n")
#endif 

/**********************************************************
*                                                TYPEDEFS *
**********************************************************/
