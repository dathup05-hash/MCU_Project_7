/*
 * =============================================================================
 *  File      : bsp_oled.h
 *  Tang      : bsp
 *  Chuc nang : Man hinh OLED GAN TREN BOARD (bus, dia chi xem bsp.h).
 *              Lenh SSD1306 / font nam o hal_oled; file nay chi noi
 *              "board nay gan OLED o bus nao, dia chi nao".
 *
 *  Cach dung (trong task hien thi):
 *      bsp_oled_init();                  // 1 lan, trong task (co cho 100 ms)
 *      bsp_oled_clear();
 *      bsp_oled_puts(0, 0, "Hello");     // cot 0 (pixel), dong 0
 *      bsp_oled_flush();
 *
 *  Gia tri tra ve: 0 = OK, khac 0 = loi I2C.
 * =============================================================================
 */
#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>

uint8_t bsp_oled_init(void);
void    bsp_oled_clear(void);
void    bsp_oled_puts(uint8_t col, uint8_t page, const char *s);
void    bsp_oled_hline(uint8_t page, uint8_t bit);
uint8_t bsp_oled_flush(void);

#endif /* BSP_OLED_H */
