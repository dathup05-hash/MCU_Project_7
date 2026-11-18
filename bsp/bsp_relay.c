/*
 * =============================================================================
 *  File      : bsp_relay.c
 *  Tang      : bsp
 *  Chuc nang : Dieu khien relay quat. Xem mo ta trong bsp_relay.h.
 * =============================================================================
 */
#include "bsp_relay.h"
#include "bsp.h"
#include "hal_gpio.h"

static uint8_t s_on = 0U;      /* trang thai hien tai: 1 = dong */

/** @brief Doi "dong/ngat" sang muc logic tren chan (tinh ca kich muc thap). */
static uint8_t prvLevel(uint8_t on)
{
#if BSP_RELAY_ACTIVE_LOW
    return (on != 0U) ? 0U : 1U;
#else
    return (on != 0U) ? 1U : 0U;
#endif
}

void bsp_relay_init(void)
{
    (void)hal_gpio_config(BSP_RELAY_PORT, BSP_RELAY_PIN, HAL_GPIO_OUT_PP);
    bsp_relay_set(0U);      /* ngat relay */
}

void bsp_relay_set(uint8_t on)
{
    s_on = (on != 0U) ? 1U : 0U;
    hal_gpio_write(BSP_RELAY_PORT, BSP_RELAY_PIN, prvLevel(s_on));
}

uint8_t bsp_relay_get(void)
{
    return s_on;
}
