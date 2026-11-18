/*
 * =============================================================================
 *  File      : main.c
 *  Du an     : He thong giam sat nhiet do va dieu khien quat tu dong
 *              STM32F103C8T6 + FreeRTOS
 *  Chuc nang : Diem bat dau chuong trinh.
 *              1. Khoi tao board (chan IO, I2C, UART debug neu bat...)
 *              2. Tao cac doi tuong IPC cua FreeRTOS (queue, mutex, event group)
 *              3. Tao 5 task
 *              4. Khoi dong bo lap lich (scheduler) cua FreeRTOS
 *
 *  UART debug: bat/tat bang BSP_DEBUG_UART_ENABLE trong bsp/bsp.h (mac dinh TAT).
 * =============================================================================
 */
#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"
#include "bsp_relay.h"
#include "bsp_uart.h"
#include "app_ipc.h"
#include "app_settings.h"
#include "task_sensor.h"
#include "task_control.h"
#include "task_input.h"
#include "task_display.h"
#include "task_alarm.h"

/**
 * @brief  Xu ly loi nghiem trong (khong tao duoc task, tran stack, het heap...).
 *         Dua he thong ve trang thai AN TOAN roi dung lai:
 *         - Tat het ngat (scheduler ngung chay)
 *         - Bat quat de khong bi qua nhiet
 *         - Bat LED PC13 de nguoi dung biet co loi
 */
static void prvFatal(void)
{
    taskDISABLE_INTERRUPTS();
    bsp_relay_set(1U);
    bsp_led_set(1U);
    for (;;) {
        /* Dung tai day. Dung debugger de xem nguyen nhan. */
    }
}

int main(void)
{
    /* ---- 1. Khoi tao board ---- */
    if (bsp_init() != 0U) {
        bsp_uart_println("BSP: loi khoi tao I2C/UART");
    }
    bsp_uart_println("=== Giam sat nhiet do - khoi dong ===");

    /* ---- 2. Tao doi tuong IPC (PHAI truoc khi tao task) ---- */
    app_settings_init();    /* nguong quat mac dinh + Mutex                */
    app_ipc_init();         /* queue nhiet do, queue quat, event group     */

    /* ---- 3. Tao cac task (tra ve pdPASS neu thanh cong) ---- */
    if (task_input_create()   != pdPASS) { prvFatal(); }   /* doc 3 nut        */
    if (task_sensor_create()  != pdPASS) { prvFatal(); }   /* doc cam bien     */
    if (task_control_create() != pdPASS) { prvFatal(); }   /* bat/tat quat     */
    if (task_alarm_create()   != pdPASS) { prvFatal(); }   /* coi + LED        */
    if (task_display_create() != pdPASS) { prvFatal(); }   /* hien thi OLED    */

    /* ---- 4. Khoi dong scheduler: tu day cac task bat dau chay ---- */
    vTaskStartScheduler();

    /* Chi chay toi day khi khong du heap de tao task Idle -> loi */
    prvFatal();
    return 0;
}

/**
 * @brief  Hook FreeRTOS tu goi khi phat hien 1 task bi TRAN STACK
 *         (bat boi configCHECK_FOR_STACK_OVERFLOW = 2 trong FreeRTOSConfig.h).
 * @note   Gap loi nay thi tang so stack cua task do trong app_cfg.h.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    prvFatal();
}

/**
 * @brief  Hook FreeRTOS tu goi khi pvPortMalloc() HET HEAP
 *         (bat boi configUSE_MALLOC_FAILED_HOOK = 1).
 * @note   Gap loi nay thi tang configTOTAL_HEAP_SIZE trong FreeRTOSConfig.h.
 */
void vApplicationMallocFailedHook(void)
{
    prvFatal();
}
