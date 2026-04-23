#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS.h"

#include "../inc/sensor_data.h"
#include "../inc/extern.h"


uint32_t g_drop_count = 0;


/* A simple task that prints a message every second */
void vSensorTask(void *pvParameters)
{
    (void)pvParameters;  /* unused */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    int count = 0;
    SensorRead_t ADC_Sensor;
    
    for (;;)
    {
        ADC_Sensor.sensorVal = 13.0f * (rand()%77)/10.03;
        ADC_Sensor.timestamp = xTaskGetTickCount();
        // debug_log(LOG_INFO, "t-stamp: %lu\ts_val: %.5f",
        //             ADC_Sensor.timestamp, ADC_Sensor.sensorVal);
        if(xQueueSend(xMyQueue, &ADC_Sensor, ( TickType_t ) 0) != pdPASS)
        {
            //Data Send unSuccessfully
            g_drop_count++;
            if(g_drop_count % 10)
            {
                debug_log(LOG_WARN, "vSensorTask sent unSuccessfull");
            }
        }
        vTaskDelayUntil(&xLastWakeTime, 100);
    }
}