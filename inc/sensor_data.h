#ifndef _SENSOR_DATA_H_
#define _SENSOR_DATA_H_


typedef struct SensorRead{
    float sensorVal;
    TickType_t timestamp;
}SensorRead_t;


void vSensorTask(void *pvParameters);



#endif