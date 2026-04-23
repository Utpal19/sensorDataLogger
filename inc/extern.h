#ifndef _EXTERN_H_
#define _EXTERN_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "debug_task.h"



extern uint32_t g_drop_count;
extern QueueHandle_t xMyQueue;
extern QueueHandle_t xPrintQueue;

extern LogLevel_t g_log_level;


#endif // _EXTERN_H_ 
