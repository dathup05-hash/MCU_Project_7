/*
 * =============================================================================
 *  File      : task_control.h
 *  Tang      : app (FreeRTOS task)
 *  Chuc nang : ControlTask - bat/tat quat (relay) theo nhiet do va nguong.
 *  Quy chuan : moi task chi dua ra ngoai DUY NHAT 1 ham tao task.
 * =============================================================================
 */
#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H

#include "FreeRTOS.h"

/**
 * @brief  Tao task (goi trong main() truoc vTaskStartScheduler()).
 * @return pdPASS neu tao thanh cong, nguoc lai la het heap.
 */
BaseType_t task_control_create(void);

#endif /* TASK_CONTROL_H */
