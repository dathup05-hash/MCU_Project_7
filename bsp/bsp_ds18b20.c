/*
 * =============================================================================
 *  File      : bsp_ds18b20.c
 *  Tang      : bsp
 *  Chuc nang : Gan driver DS18B20 (hal_ds18b20) vao chan cua board.
 * =============================================================================
 */
#include "bsp_ds18b20.h"
#include "bsp.h"
#include "hal_ds18b20.h"

void bsp_ds18b20_init(void)
{
    (void)hal_ds18b20_init(BSP_OW_PORT, BSP_OW_PIN);
}

uint8_t bsp_ds18b20_start_conversion(void)
{
    return hal_ds18b20_start_conversion();
}

uint8_t bsp_ds18b20_read_temp(int16_t *temp_x10)
{
    return hal_ds18b20_read_temp(temp_x10);
}
