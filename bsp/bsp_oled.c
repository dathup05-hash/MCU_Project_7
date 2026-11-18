/*
 * =============================================================================
 *  File      : bsp_oled.c
 *  Tang      : bsp
 *  Chuc nang : Gan driver OLED (hal_oled) vao bus I2C / dia chi cua board.
 * =============================================================================
 */
#include "bsp_oled.h"
#include "bsp.h"
#include "hal_oled.h"

uint8_t bsp_oled_init(void)
{
    return hal_oled_init(BSP_OLED_I2C, BSP_OLED_ADDR, BSP_OLED_COL_OFFSET);
}

void bsp_oled_clear(void)
{
    hal_oled_clear();
}

void bsp_oled_puts(uint8_t col, uint8_t page, const char *s)
{
    hal_oled_puts(col, page, s);
}

void bsp_oled_hline(uint8_t page, uint8_t bit)
{
    hal_oled_hline(page, bit);
}

uint8_t bsp_oled_flush(void)
{
    return hal_oled_flush();
}
