/*
 * =============================================================================
 *  File      : task_display.h
 *  Tang      : app (FreeRTOS task)
 *  Chuc nang : DisplayTask - ve thong tin len man hinh OLED.
 *  Quy chuan : moi task chi dua ra ngoai DUY NHAT 1 ham tao task.
 * =============================================================================
 */
#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H

#include "FreeRTOS.h"

/**
 * @brief  Tao task (goi trong main() truoc vTaskStartScheduler()).
 * @return pdPASS neu tao thanh cong, nguoc lai la het heap.
 */
BaseType_t task_display_create(void);

#endif /* TASK_DISPLAY_H */
