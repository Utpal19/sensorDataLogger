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
    LogNode *tail, *temp_head;
    LogNode *temp = (LogNode*) pvPortMalloc(sizeof(LogNode));
    if(temp == NULL)
    {
        exit(EXIT_FAILURE);
    }
    temp->data = *(SensorRead_t*)data;
    temp->next = NULL;

    // Mutex only around pointer manipulation
    xSemaphoreTake(xListMutex, portMAX_DELAY);
    if(head == NULL)
    {
        head = temp;
        nodeCount++;
    }
    else
    {
        tail = head;
        while(tail->next != NULL)
        {
            tail = tail->next;
        }
        tail->next = temp;
        nodeCount++;
        // debug_log(LOG_INFO, "nodeCount: %d", nodeCount);
    }
    while(nodeCount >= (NODE_POOL_LEN+1))
    {
        temp_head = head;
        head = head->next;
        xSemaphoreGive(xListMutex);
        vPortFree(temp_head);   // free OUTSIDE mutex
        xSemaphoreTake(xListMutex, portMAX_DELAY);
        nodeCount--;
    }
    xSemaphoreGive(xListMutex);
        

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
            appendNode(&receivedData);
        }
        else
        {
            debug_log(LOG_WARN, "vLoggerTask Queue rcv failed");
        }
    }
}


uint8_t getLogSnapshot(SensorRead_t *buf, uint8_t bufLen)
{
    uint8_t count = 0;
    
    xSemaphoreTake(xListMutex, portMAX_DELAY);
    LogNode *cur = head;
    // while(cur->next != NULL && count <= bufLen)
    while(cur != NULL && count <= bufLen)
    {
        buf[count++] = cur->data;
        cur = cur->next;
    }
    xSemaphoreGive(xListMutex);
    
    return count;
}