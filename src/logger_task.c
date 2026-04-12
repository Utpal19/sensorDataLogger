#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS.h"

#include "../inc/sensor_data.h"
#include "../inc/logger_task.h"
#include "../inc/extern.h"


typedef struct Node{
    SensorRead_t sPkt;
    struct Node *next;
}Node;

Node *head = NULL;




void vLoggerTask(void *pvParameters)
{
    while(1)
    {
        SensorRead_t receivedData;
        if(xQueueReceive(xMyQueue, &receivedData, portMAX_DELAY) == pdPASS)
        {
            // Process the received data (e.g., log it to a file or print it)
            printf("** TimeStamp: %lu\tADC Val: %.5f **\n", 
                    receivedData.timestamp, receivedData.sensorVal);
            fflush(stdout);
        }
    }
}