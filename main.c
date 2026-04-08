#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"

/* Required by configASSERT in FreeRTOSConfig.h */
void vAssertCalled( const char * const pcFileName, unsigned long ulLine )
{
    fprintf( stderr, "ASSERT failed: %s, line %lu\n", pcFileName, ulLine );
    abort();
}

/* Required by configGENERATE_RUN_TIME_STATS in FreeRTOSConfig.h */
void vConfigureTimerForRunTimeStats( void ) { }
unsigned long ulGetRunTimeCounterValue( void ) { return 0; }
//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//


// TaskHandle_t 





/* A simple task that prints a message every second */
static void vSensorTask(void *pvParameters)
{
    (void)pvParameters;  /* unused */
    int count = 0;
    
    for (;;)
    {
        printf("*Hello from task! count = %d*\n", count++);
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(500));  /* sleep 500 mili second */
    }
}

int main(void)
{
    printf("Starting FreeRTOS scheduler...\n");
    fflush(stdout);
    
    /* Create one task */
    xTaskCreate(vSensorTask, 
        "Sensor-Task",
        200,
        NULL,
        4,
        NULL
    );
    
    /* Start the scheduler - this does not return */
    vTaskStartScheduler();
    
    /* Should never reach here */
    for (;;);
    return 0;
}






//-------------------------------------------------------------------------//
//-------------------------------------------------------------------------//
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