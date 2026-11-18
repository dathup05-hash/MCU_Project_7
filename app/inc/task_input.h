/*
 * =============================================================================
 *  File      : task_input.h
 *  Tang      : app (FreeRTOS task)
 *  Chuc nang : InputTask - doc 3 nut nhan SET/UP/DOWN de cai dat nguong.
 *  Quy chuan : moi task chi dua ra ngoai DUY NHAT 1 ham tao task.
 * =============================================================================
 */
#ifndef TASK_INPUT_H
#define TASK_INPUT_H

#include "FreeRTOS.h"

/**
 * @brief  Tao task (goi trong main() truoc vTaskStartScheduler()).
 * @return pdPASS neu tao thanh cong, nguoc lai la het heap.
 */
BaseType_t task_input_create(void);

#endif /* TASK_INPUT_H */
