#ifndef _LOGGER_TASK_H_
#define _LOGGER_TASK_H_

#include "../inc/sensor_data.h"

void vLoggerTask(void *pvParameters);
uint8_t getLogSnapshot(SensorRead_t *buf, uint8_t bufLen);

#endif