#ifndef _EXTERN_H_
#define _EXTERN_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "debug_task.h"


#define NODE_POOL_LEN   100

extern uint32_t g_drop_count;
extern uint32_t g_print_drop_count;
extern QueueHandle_t xMyQueue;
extern QueueHandle_t xPrintQueue;

extern SemaphoreHandle_t xListMutex;

extern LogLevel_t g_log_level;


#endif // _EXTERN_H_ 
