/*
 * =============================================================================
 *  File      : bsp.c
 *  Tang      : bsp (Board Support Package)
 *  Chuc nang : Khoi tao ca board (bsp_init) + LED canh bao + coi.
 *              Nut, relay, UART, DS18B20, OLED nam o file rieng (xem bsp.h).
 *
 *  Vi du: app goi bsp_buzzer_set(1)
 *         -> bsp biet coi o PB12, kich muc cao
 *         -> hal_gpio_write(GPIOB, GPIO_Pin_12, 1)
 *         -> SPL GPIO_SetBits()
 * =============================================================================
 */
#include "bsp.h"
#include "bsp_button.h"
#include "bsp_relay.h"
#include "bsp_uart.h"
#include "bsp_ds18b20.h"
#include "hal_gpio.h"
#include "hal_i2c.h"

uint8_t bsp_init(void)
{
    uint8_t err = 0U;

    /* SystemInit() (file startup goi truoc main) da dat clock 72 MHz.
     * Cap nhat bien SystemCoreClock cho cac ham tinh toc do (I2C, UART). */
    SystemCoreClockUpdate();

    /* FreeRTOS tren Cortex-M3 yeu cau dung ca 4 bit uu tien ngat cho
     * "preemption priority" (Group 4). */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* 1. Output: dua ve trang thai an toan (tat het) */
    (void)hal_gpio_config(BSP_LED_PORT, BSP_LED_PIN, HAL_GPIO_OUT_PP);
    bsp_led_set(0U);
    (void)hal_gpio_config(BSP_BUZZER_PORT, BSP_BUZZER_PIN, HAL_GPIO_OUT_PP);
    bsp_buzzer_set(0U);
    bsp_relay_init();

    /* 2. Input */
    bsp_button_init();
    bsp_ds18b20_init();

    /* 3. Bus giao tiep */
    (void)hal_gpio_config(BSP_OLED_I2C_PORT, BSP_OLED_SCL_PIN | BSP_OLED_SDA_PIN, HAL_GPIO_AF_OD);
    err |= hal_i2c_init(BSP_OLED_I2C, BSP_OLED_I2C_SPEED);
    err |= bsp_uart_init();

    return (err != 0U) ? 1U : 0U;
}

void bsp_led_set(uint8_t on)
{
#if BSP_LED_ACTIVE_LOW
    hal_gpio_write(BSP_LED_PORT, BSP_LED_PIN, (on != 0U) ? 0U : 1U);
#else
    hal_gpio_write(BSP_LED_PORT, BSP_LED_PIN, (on != 0U) ? 1U : 0U);
#endif
}

void bsp_buzzer_set(uint8_t on)
{
    hal_gpio_write(BSP_BUZZER_PORT, BSP_BUZZER_PIN, (on != 0U) ? 1U : 0U);
}
