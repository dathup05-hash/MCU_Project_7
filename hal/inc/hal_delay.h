/*
 * =============================================================================
 *  File      : hal_delay.h
 *  Tang      : hal
 *  Chuc nang : 3 ham thoi gian.
 *
 *  1. Delay CHAN chuong trinh (2 vong for long nhau, CPU quay vong):
 *       hal_delay_us() : dung cho thoi gian rat ngan (1-Wire DS18B20, timeout).
 *                        Dung duoc ca truoc va sau khi scheduler chay.
 *
 *  2. Delay dung FreeRTOS:
 *       hal_delay_ms() : task NGU va nhuong CPU cho task khac (vTaskDelay).
 *       hal_millis()   : so ms tu luc scheduler chay (bo dem tick, 1 tick = 1 ms).
 *     Khong tu cau hinh SysTick vi FreeRTOS da dung SysTick lam nhip he thong.
 * =============================================================================
 */
#ifndef HAL_DELAY_H
#define HAL_DELAY_H

#include <stdint.h>

/**
 * @brief Cho ban `us` micro-giay bang 2 vong for long nhau (chan chuong trinh).
 * @note  Do chinh xac phu thuoc HAL_DELAY_LOOPS_PER_US trong hal_delay.c
 *        (hieu chinh tren kit). Bi ngat chen vao thi thoi gian dai them.
 */
void     hal_delay_us(uint32_t us);

/**
 * @brief Cho `ms` mili-giay. Trong task: task ngu, task khac duoc chay.
 * @note  Neu goi khi scheduler chua chay (trong main) thi tu chuyen sang cho ban.
 *        Khong goi trong ngat.
 */
void     hal_delay_ms(uint32_t ms);

/**
 * @brief  So mili-giay ke tu khi vTaskStartScheduler() duoc goi.
 * @return Luon = 0 truoc khi scheduler chay. Tran ve 0 sau ~49 ngay.
 * @note   Khong goi trong ngat.
 */
uint32_t hal_millis(void);

#endif /* HAL_DELAY_H */
