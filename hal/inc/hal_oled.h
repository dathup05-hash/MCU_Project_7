/*
 * =============================================================================
 *  File      : hal_oled.h
 *  Tang      : hal (driver thiet bi ngoai)
 *  Chuc nang : Driver man hinh OLED 0.96" 128x64 chip SSD1306, giao tiep I2C (chi ghi).
 *              File nay KHONG biet board: bus I2C, dia chi do tang bsp truyen vao.
 *
 *  Man hinh chia thanh 8 "page" (dong), moi page cao 8 pixel.
 *  Font 5x7, moi ky tu chiem 6 pixel ngang -> 21 ky tu/dong, 8 dong.
 *
 *  Cach dung: VE vao bo dem RAM truoc, roi FLUSH 1 lan len man hinh
 *      hal_oled_init(I2C1, 0x3C, 0);     // 1 lan (bsp lam), I2C phai init truoc
 *      hal_oled_clear();
 *      hal_oled_puts(0, 0, "Hello");     // cot 0 (pixel), dong 0
 *      hal_oled_flush();                 // gui 1 KB qua I2C (~25 ms o 400 kHz)
 *
 *  Ky tu '\x7F' trong chuoi = dau do.
 *  Gia tri tra ve: 0 = OK, khac 0 = ma loi HAL_I2C_ERR_xxx (xem hal_i2c.h).
 * =============================================================================
 */
#ifndef HAL_OLED_H
#define HAL_OLED_H

#include <stdint.h>
#include "stm32f10x.h"

#define OLED_WIDTH      128U    /* so cot pixel  */
#define OLED_PAGES      8U      /* so dong (page) */

/**
 * @brief  Cho nguon on dinh, gui chuoi lenh khoi tao, bat man hinh.
 * @param  i2c        Bus I2C da khoi tao (I2C1 / I2C2)
 * @param  addr7      Dia chi 7 bit (thuong 0x3C)
 * @param  col_offset Lech cot: SSD1306 0.96" = 0, SH1106 1.3" = 2
 * @return 0 = OK, khac 0 = loi I2C (chua cam OLED / sai dia chi)
 * @note   Co cho 100 ms (hal_delay_ms) -> nen goi trong task.
 */
uint8_t hal_oled_init(I2C_TypeDef *i2c, uint8_t addr7, uint8_t col_offset);

/** @brief Xoa bo dem RAM (man hinh chua doi cho toi khi flush). */
void    hal_oled_clear(void);

/**
 * @brief Ve chuoi vao bo dem.
 * @param col  Cot bat dau, tinh bang PIXEL (0..127). Ky tu thu n o cot n*6.
 * @param page Dong (0..7)
 * @param s    Chuoi ASCII ket thuc '\0'. Tran mep phai thi bi cat.
 */
void    hal_oled_puts(uint8_t col, uint8_t page, const char *s);

/**
 * @brief Ke 1 duong ngang het chieu rong man hinh.
 * @param page Dong (0..7)
 * @param bit  Vi tri pixel trong dong (0 = tren cung, 7 = duoi cung)
 */
void    hal_oled_hline(uint8_t page, uint8_t bit);

/** @brief Day toan bo bo dem RAM len man hinh. @return 0 = OK, khac 0 = loi I2C */
uint8_t hal_oled_flush(void);

#endif /* HAL_OLED_H */
