#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "FreeRTOS.h"

#include "../inc/sensor_data.h"
#include "../inc/logger_task.h"
#include "../inc/extern.h"


/* append to tail, if count > 100 drop from head */
typedef struct LogNode{
    SensorRead_t data;
    struct LogNode *next;
}LogNode;

LogNode *head = NULL;

void appendNode(void *data)
{
    static uint8_t nodeCount = 0;
    LogNode *currentNode;
    LogNode *temp = (LogNode*) pvPortMalloc(sizeof(LogNode));
    temp->data = *(SensorRead_t*)data;
    temp->next = NULL;

    if (head == NULL) {
        head = temp;
        nodeCount++;
        return;
    }
    currentNode = head;

    if(nodeCount == 100)
    {
        temp->next = currentNode->next;
        head = temp;
        nodeCount--;
        debug_log(LOG_INFO, "*** Replaced head node ***\r\n");
    }
    else if(nodeCount < 100)
    {
        while(currentNode->next != NULL)
        {
            currentNode = currentNode->next;
        }
        currentNode->next = temp;
        nodeCount++;
    }   
}

void vLoggerTask(void *pvParameters)
{
    while(1)
    {
        SensorRead_t receivedData;
        if(xQueueReceive(xMyQueue, &receivedData, portMAX_DELAY) == pdPASS)
        {
            // Process the received data (e.g., log it to a file or print it)
            // debug_log(LOG_INFO, "** TimeStamp: %lu\tADC Val: %.5f **",
            //         receivedData.timestamp, receivedData.sensorVal);
            // appendNode(&receivedData);
        }
        else
        {
            debug_log(LOG_WARN, "vLoggerTask Queue rcv failed");
        }
    }
}