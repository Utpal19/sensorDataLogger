/*
* We can use this make command also to make this project: 
* make NO_TRACING=1
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "../inc/sensor_data.h"
#include "../inc/logger_task.h"
#include "../inc/debug_task.h"
#include "../inc/extern.h"


/* Required by configASSERT in FreeRTOSConfig.h */
void vAssertCalled( const char * const pcFileName, unsigned long ulLine )
{
    fprintf( stderr, "ASSERT failed: %s, line %lu\n", pcFileName, ulLine );
    abort();
}

/* Required by TRC_HARDWARE_PORT_Win32 in trcHardwarePort.h.
 * Use clock_gettime so these are safe to call before the scheduler starts. */
static uint64_t ullTraceEntryNs = 0U;
static uint64_t prvGetNs( void )
{
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return ( uint64_t ) ts.tv_sec * 1000000000ULL + ( uint64_t ) ts.tv_nsec;
}
void vTraceTimerReset( void )             { ullTraceEntryNs = prvGetNs(); }
uint32_t uiTraceTimerGetFrequency( void ) { return 1000000U; /* 1 MHz — report in microseconds */ }
uint32_t uiTraceTimerGetValue( void )     { return ( uint32_t )( ( prvGetNs() - ullTraceEntryNs ) / 1000U ); }
//-------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//


LogLevel_t g_log_level = LOG_INFO;

TaskHandle_t xSensorTaskHandle;
TaskHandle_t xLoggerTaskHandle;
TaskHandle_t xDebugTaskHandle;

QueueHandle_t xMyQueue = NULL;
QueueHandle_t xPrintQueue = NULL;

#define PRINT_QUEUE_LEN 10


int main(void)
{
#if ( projENABLE_TRACING == 1 )
    vTraceEnable( TRC_START );
#endif

    printf("Starting FreeRTOS scheduler...\n");
    fflush(stdout);

    BaseType_t status;

    xMyQueue = xQueueCreate(10, sizeof(SensorRead_t));
    configASSERT(xMyQueue != NULL);
    
    xPrintQueue = xQueueCreate(PRINT_QUEUE_LEN, sizeof(print_msg_t));
    configASSERT(xPrintQueue != NULL);

    /* Create one task */
    status = xTaskCreate(vSensorTask, 
        "Sensor-Task",
        200,
        NULL,
        4,
        &xSensorTaskHandle
    );
    configASSERT(status ==pdPASS);

    xTaskCreate(vLoggerTask, 
        "Logger-Task",
        200,
        NULL,
        3,
        &xLoggerTaskHandle
    );
    configASSERT(status == pdPASS);

    xTaskCreate(vDebugTask, 
        "Debug-Task",
        200,
        NULL,
        1,
        &xLoggerTaskHandle
    );
    configASSERT(status == pdPASS);
    
    /* Start the scheduler - this does not return */
    vTaskStartScheduler();
    
    /* Should never reach here */
    for (;;);
    return 0;
}






//-------------------------------------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------------------------------------//
/* FreeRTOS hooks - required by the kernel, can be empty */
void vApplicationMallocFailedHook(void) { for(;;); }
void vApplicationIdleHook(void) { }
void vApplicationTickHook(void) { }
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pxTask; (void)pcTaskName;
    for(;;);
}
void vApplicationDaemonTaskStartupHook(void) { }
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    static StaticTask_t xIdleTaskTCB;
    static StackType_t  uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    *ppxIdleTaskTCBBuffer   = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t **ppxTimerTaskStackBuffer,
                                     uint32_t *pulTimerTaskStackSize )
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t  uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    *ppxTimerTaskTCBBuffer   = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize   = configTIMER_TASK_STACK_DEPTH;
}