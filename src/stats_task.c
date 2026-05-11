#include <stdio.h>
#include "FreeRTOS.h"

#include "../inc/logger_task.h"
#include "../inc/sensor_data.h"
#include "../inc/stats_task.h"
#include "../inc/extern.h"

void vStatsTask(void *pvParmaeter)
{
    uint8_t count = 0, len = 0;
    SensorRead_t s_val[NODE_POOL_LEN]; 
    float avg_val = 0, min_val = 0, max_val = 0;
    while(1)
    {
        len = 0;
        avg_val = 0;

        count = getLogSnapshot(&s_val[0], NODE_POOL_LEN);
        if(len == 0)
        {
            min_val = s_val[0].sensorVal;
            max_val = s_val[0].sensorVal;
        }
        while(len < count)
        {
            if(min_val > s_val[len].sensorVal)
                min_val = s_val[len].sensorVal;
            if(s_val[len].sensorVal > max_val)
                max_val = s_val[len].sensorVal;
            avg_val = avg_val + ((s_val[len].sensorVal - avg_val)/(len+1));
            len++;
        }
        debug_log(LOG_INFO, "Count: %d\tAvg_val: %.3f\tMin_Val: %.3f\tMax_Val: %.3f\r\n",
                count, avg_val, min_val, max_val);

        // compute stats on snapshot[]...
        vTaskDelay(pdMS_TO_TICKS(2000));
    }


}