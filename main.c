#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

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


TaskHandle_t xSensorTaskHandle;
QueueHandle_t xMyQueue = NULL;

typedef struct SensorRead{
    float sensorVal;
    TickType_t timestamp;
}SensorRead_t;


/* A simple task that prints a message every second */
static void vSensorTask(void *pvParameters)
{
    (void)pvParameters;  /* unused */
    int count = 0;
    SensorRead_t ADC_Sensor;
    
    for (;;)
    {
        ADC_Sensor.sensorVal = 13 * (rand()%77)/10.03;
        ADC_Sensor.timestamp = xTaskGetTickCount();
        printf("TimeStamp: %lu\tADC Val: %.5f\n", 
                ADC_Sensor.timestamp, ADC_Sensor.sensorVal);
        fflush(stdout);
        if(xQueueSend(xMyQueue, &ADC_sensor, pdMS_TO_TICKS(100)) != pdPASS)
        {
            //Data Send unSuccessfully
            printf("Data Send unSuccessfully\n");
            fflush(stdout);
        }
        // vTaskDelay(pdMS_TO_TICKS(100)); // Sleep 100 ms //
    }
}

int main(void)
{
    printf("Starting FreeRTOS scheduler...\n");
    fflush(stdout);

    BaseType_t status;
    xMyQueue = xQueueCreate(10, sizeof(SensorRead_t));
    configASSERT(xMyQueue != NULL);
    
    /* Create one task */
    status = xTaskCreate(vSensorTask, 
        "Sensor-Task",
        200,
        NULL,
        4,
        &xSensorTaskHandle
    );
    configASSERT(status ==pdPASS);
    
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