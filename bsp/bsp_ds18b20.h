/*
 * =============================================================================
 *  File      : bsp_ds18b20.h
 *  Tang      : bsp
 *  Chuc nang : Cam bien nhiet do DS18B20 GAN TREN BOARD (chan xem bsp.h).
 *              Giao thuc 1-Wire nam o hal_ds18b20; file nay chi noi
 *              "board nay gan DS18B20 o chan nao".
 *
 *  Cach dung (trong task):
 *      bsp_ds18b20_start_conversion();         // ra lenh do
 *      vTaskDelay(pdMS_TO_TICKS(750));         // cho cam bien do xong
 *      bsp_ds18b20_read_temp(&t);              // doc ket qua, t = 285 -> 28.5 C
 *
 *  Gia tri tra ve: 0 = thanh cong, 1 = loi (khong co cam bien / sai CRC).
 * =============================================================================
 */
#ifndef BSP_DS18B20_H
#define BSP_DS18B20_H

#include <stdint.h>

/** @brief Gan DS18B20 vao chan cua board. Da duoc goi trong bsp_init(). */
void    bsp_ds18b20_init(void);

/** @brief Ra lenh do. @return 0 = OK, 1 = khong co cam bien tra loi */
uint8_t bsp_ds18b20_start_conversion(void);

/** @brief Doc ket qua. @param temp_x10 [out] x10 do C. @return 0 = OK, 1 = loi */
uint8_t bsp_ds18b20_read_temp(int16_t *temp_x10);

#endif /* BSP_DS18B20_H */
