/*
 * =============================================================================
 *  File      : task_sensor.h
 *  Tang      : app (FreeRTOS task)
 *  Chuc nang : TemperatureTask - doc cam bien DS18B20 moi 1 giay.
 *  Quy chuan : moi task chi dua ra ngoai DUY NHAT 1 ham tao task.
 * =============================================================================
 */
#ifndef TASK_SENSOR_H
#define TASK_SENSOR_H

#include "FreeRTOS.h"

/**
 * @brief  Tao task (goi trong main() truoc vTaskStartScheduler()).
 * @return pdPASS neu tao thanh cong, nguoc lai la het heap.
 */
BaseType_t task_sensor_create(void);

#endif /* TASK_SENSOR_H */
