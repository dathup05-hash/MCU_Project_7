/*
 * =============================================================================
 *  File      : task_alarm.h
 *  Tang      : app (FreeRTOS task)
 *  Chuc nang : AlarmTask - coi + LED canh bao (qua nhiet 40 C / mat cam bien),
 *              bip khi bam nut, bip khi quat bat.
 *  Quy chuan : moi task chi dua ra ngoai DUY NHAT 1 ham tao task.
 * =============================================================================
 */
#ifndef TASK_ALARM_H
#define TASK_ALARM_H

#include "FreeRTOS.h"

/**
 * @brief  Tao task (goi trong main() truoc vTaskStartScheduler()).
 * @return pdPASS neu tao thanh cong, nguoc lai la het heap.
 */
BaseType_t task_alarm_create(void);

#endif /* TASK_ALARM_H */
