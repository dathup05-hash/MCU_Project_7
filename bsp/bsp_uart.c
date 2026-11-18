/*
 * =============================================================================
 *  File      : bsp_uart.c
 *  Tang      : bsp
 *  Chuc nang : UART debug. Xem mo ta trong bsp_uart.h.
 *              Bat/tat bang BSP_DEBUG_UART_ENABLE trong bsp.h. Khi TAT:
 *              khong cau hinh chan PA9/PA10, cac ham in/doc khong lam gi.
 * =============================================================================
 */
#include <stddef.h>
#include "bsp_uart.h"
#include "bsp.h"
#include "hal_gpio.h"
#include "hal_uart.h"

#if BSP_DEBUG_UART_ENABLE

uint8_t bsp_uart_init(void)
{
    /* TX = chuc nang phu push-pull (UART dieu khien chan), RX = input keo len */
    (void)hal_gpio_config(BSP_DEBUG_PORT, BSP_DEBUG_TX_PIN, HAL_GPIO_AF_PP);
    (void)hal_gpio_config(BSP_DEBUG_PORT, BSP_DEBUG_RX_PIN, HAL_GPIO_IN_PULLUP);
    return hal_uart_init(BSP_DEBUG_UART, BSP_DEBUG_BAUD);
}

void bsp_uart_print(const char *s)
{
    hal_uart_puts(BSP_DEBUG_UART, s);
}

void bsp_uart_println(const char *s)
{
    hal_uart_println(BSP_DEBUG_UART, s);
}

uint16_t bsp_uart_read_line(char *buf, uint16_t size, uint32_t timeout_ms)
{
    return hal_uart_gets(BSP_DEBUG_UART, buf, size, timeout_ms);
}

#else   /* UART debug TAT: giu nguyen ten ham de cac task khong phai sua */

uint8_t bsp_uart_init(void)
{
    return 0U;
}

void bsp_uart_print(const char *s)
{
    (void)s;
}

void bsp_uart_println(const char *s)
{
    (void)s;
}

uint16_t bsp_uart_read_line(char *buf, uint16_t size, uint32_t timeout_ms)
{
    (void)size;
    (void)timeout_ms;
    if (buf != NULL) {
        buf[0] = '\0';
    }
    return 0U;
}

#endif
