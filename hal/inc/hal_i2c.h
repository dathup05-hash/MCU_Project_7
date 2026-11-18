/*
 * =============================================================================
 *  File      : hal_i2c.h
 *  Tang      : hal
 *  Chuc nang : I2C master CHI GHI, dang polling (CPU tu cho tung buoc), boc SPL.
 *                - Cau hinh : hal_i2c_init()
 *                - Ghi      : hal_i2c_write()
 *              Du an chi co OLED SSD1306 dung I2C, ma OLED chi can ghi
 *              -> khong co ham doc. Can doc (cam bien I2C...) thi bo sung sau.
 *
 *  An toan khi dung FreeRTOS:
 *    - Moi bus co 1 Mutex: 2 task cung dung 1 bus thi task sau phai cho.
 *    - Moi buoc deu co timeout -> khong bao gio treo vinh vien.
 *    - KHONG goi trong ngat (vi co Mutex).
 *
 *  Chan (cau hinh bang hal_gpio_config(..., HAL_GPIO_AF_OD) TRUOC khi init):
 *    I2C1: SCL = PB6,  SDA = PB7
 *    I2C2: SCL = PB10, SDA = PB11
 *
 *  Dia chi thiet bi: 7 bit, CHUA dich trai (vd OLED SSD1306 = 0x3C).
 *  Kiem tra thiet bi co cam khong: hal_i2c_write(I2C1, 0x3C, 0x00, NULL, 0, 10)
 *    -> HAL_I2C_OK = co tra loi, HAL_I2C_ERR_NACK = khong co thiet bi.
 * =============================================================================
 */
#ifndef HAL_I2C_H
#define HAL_I2C_H

#include <stdint.h>
#include "stm32f10x.h"

/* -----------------------------------------------------------------------------
 *  Ma ket qua tra ve cua hal_i2c_write
 * -------------------------------------------------------------------------- */
#define HAL_I2C_OK              0U  /* thanh cong                                        */
#define HAL_I2C_ERR_PARAM       1U  /* tham so sai, hoac bus chua init                   */
#define HAL_I2C_ERR_NACK        2U  /* thiet bi khong tra loi (sai dia chi / chua cam)   */
#define HAL_I2C_ERR_TIMEOUT     3U  /* qua thoi gian cho (day long, thieu tro keo len)   */
#define HAL_I2C_ERR_BUS         4U  /* loi duong truyen (nhieu) hoac mat quyen dieu khien bus */
#define HAL_I2C_ERR_BUSY        5U  /* task khac dang dung bus qua lau                    */

/**
 * @brief  Khoi tao bus I2C o che do master, dia chi 7 bit. Tao Mutex cho bus.
 * @param  i2c      I2C1 hoac I2C2
 * @param  speed_hz 100000 (chuan) hoac 400000 (nhanh)
 * @return 0 = OK, 1 = tham so sai / khong tao duoc Mutex (het heap)
 * @note   Goi trong main() truoc vTaskStartScheduler().
 */
uint8_t hal_i2c_init(I2C_TypeDef *i2c, uint32_t speed_hz);

/**
 * @brief  Ghi:  START | addr+W | reg | data[0..len-1] | STOP
 * @param  addr7      Dia chi 7 bit cua thiet bi
 * @param  reg        Byte dau tien gui di (dia chi thanh ghi, hoac byte control cua OLED)
 * @param  data       Du lieu gui sau reg (NULL neu len = 0)
 * @param  len        So byte data (0 = chi gui reg)
 * @param  timeout_ms Thoi gian toi da (xap xi) cho Mutex, va rieng cho phan truyen
 *                    tren day (toi da 50000 ms)
 * @return HAL_I2C_OK hoac HAL_I2C_ERR_xxx
 */
uint8_t hal_i2c_write(I2C_TypeDef *i2c, uint8_t addr7, uint8_t reg,
                      const uint8_t *data, uint16_t len, uint32_t timeout_ms);

#endif /* HAL_I2C_H */
