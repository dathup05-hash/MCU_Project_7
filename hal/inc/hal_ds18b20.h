/*
 * =============================================================================
 *  File      : hal_ds18b20.h
 *  Tang      : hal (driver thiet bi ngoai)
 *  Chuc nang : Driver cam bien nhiet do DS18B20 (giao tiep 1-Wire, 1 day du lieu).
 *              STM32F1 khong co phan cung 1-Wire -> tu tao tin hieu bang GPIO
 *              + hal_delay_us (bit-banging).
 *              Chi ho tro 1 cam bien tren bus (dung lenh Skip ROM).
 *              Do phan giai mac dinh 12 bit = 0.0625 do C, do mat 750 ms.
 *
 *  File nay KHONG biet board: chan DQ do tang bsp truyen vao qua init.
 *
 *  Cach dung:
 *      hal_ds18b20_init(GPIOA, GPIO_Pin_1);     // 1 lan (bsp lam)
 *      hal_ds18b20_start_conversion();          // ra lenh do
 *      vTaskDelay(pdMS_TO_TICKS(750));          // cho cam bien do xong
 *      hal_ds18b20_read_temp(&t);               // doc ket qua, t = 285 -> 28.5 C
 *
 *  Gia tri tra ve: 0 = thanh cong, 1 = loi.
 * =============================================================================
 */
#ifndef HAL_DS18B20_H
#define HAL_DS18B20_H

#include <stdint.h>
#include "stm32f10x.h"

/**
 * @brief  Chon chan DQ, cau hinh open-drain va tha bus.
 * @param  port GPIOA..GPIOD
 * @param  pin  Chan DQ, vd GPIO_Pin_1 (can tro keo len 4.7k ve 3.3V)
 * @return 0 = OK, 1 = tham so sai
 */
uint8_t hal_ds18b20_init(GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief  Ra lenh cam bien bat dau do nhiet do (sau do phai cho 750 ms).
 * @return 0 = OK, 1 = chua init / khong co cam bien tra loi
 */
uint8_t hal_ds18b20_start_conversion(void);

/**
 * @brief  Doc ket qua lan do truoc.
 * @param  temp_x10 [out] nhiet do, don vi 0.1 do C (285 = 28.5 C)
 * @return 0 = OK, 1 = chua init / khong co cam bien / du lieu sai CRC
 */
uint8_t hal_ds18b20_read_temp(int16_t *temp_x10);

#endif /* HAL_DS18B20_H */
