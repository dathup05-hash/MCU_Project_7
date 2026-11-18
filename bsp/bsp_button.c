/*
 * =============================================================================
 *  File      : bsp_button.c
 *  Tang      : bsp
 *  Chuc nang : 3 nut nhan + ngat EXTI. Xem mo ta trong bsp_button.h.
 *              Doi "chan GPIO gay ngat" thanh "ID nut" roi goi ham cua app.
 * =============================================================================
 */
#include "bsp_button.h"
#include "bsp.h"
#include "hal_gpio.h"

/* Bang tra cuu: ID nut -> chan GPIO (thu tu theo BSP_BTN_UP/DOWN/RESET) */
static const uint16_t s_btn_pins[BSP_BTN_COUNT] = {
    BSP_BTN_UP_PIN, BSP_BTN_DOWN_PIN, BSP_BTN_RESET_PIN
};

static bsp_button_cb_t s_cb = 0;    /* ham cua tang app */

void bsp_button_init(void)
{
    uint8_t i;

    for (i = 0U; i < BSP_BTN_COUNT; i++) {
        (void)hal_gpio_config(BSP_BTN_PORT, s_btn_pins[i], HAL_GPIO_IN_PULLUP);
    }
}

/** @brief Ham hal goi trong ngat: doi chan -> ID nut, goi tiep ham cua app. */
static void prvExtiCb(uint16_t pin)
{
    uint8_t i;

    for (i = 0U; i < BSP_BTN_COUNT; i++) {
        if (s_btn_pins[i] == pin && s_cb != 0) {
            s_cb(i);
        }
    }
}

void bsp_button_irq_init(bsp_button_cb_t cb)
{
    uint8_t i;

    s_cb = cb;
    for (i = 0U; i < BSP_BTN_COUNT; i++) {
        /* Bam nut = chan tu 1 xuong 0 -> ngat canh xuong */
        (void)hal_exti_config(BSP_BTN_PORT, s_btn_pins[i], HAL_EXTI_FALLING,
                              BSP_BTN_IRQ_PRIO, prvExtiCb);
    }
}

uint8_t bsp_button_pressed(uint8_t btn)
{
    if (btn >= BSP_BTN_COUNT) {
        return 0U;
    }
    return (hal_gpio_read(BSP_BTN_PORT, s_btn_pins[btn]) == 0U) ? 1U : 0U;
}
