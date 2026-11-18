/*
 * =============================================================================
 *  File      : hal_oled.c
 *  Tang      : hal (driver thiet bi ngoai)
 *  Chuc nang : Driver OLED SSD1306. Xem cach dung trong hal_oled.h.
 *              Bus I2C, dia chi, lech cot duoc truyen vao qua hal_oled_init()
 *              -> file nay KHONG biet board.
 *
 *  Giao thuc I2C cua SSD1306: moi lan gui bat dau bang 1 byte "control":
 *    0x00 = cac byte sau la LENH
 *    0x40 = cac byte sau la DU LIEU pixel
 *
 *  Bo dem s_fb[page][cot]: moi byte = 1 cot doc 8 pixel, bit 0 o tren cung.
 * =============================================================================
 */
#include <string.h>
#include "hal_oled.h"
#include "hal_oled_font.h"
#include "hal_i2c.h"
#include "hal_delay.h"

#define OLED_CTRL_CMD   0x00    /* byte control: gui lenh     */
#define OLED_CTRL_DATA  0x40    /* byte control: gui du lieu  */
#define OLED_TIMEOUT_MS 50U     /* timeout moi lan gui I2C    */
#define OLED_POWERUP_MS 100U    /* cho nguon OLED on dinh     */

/* Bo dem khung hinh (framebuffer) 128 x 64 pixel = 1024 byte trong RAM */
static uint8_t s_fb[OLED_PAGES][OLED_WIDTH];

/* Thong tin ket noi (luu khi init) */
static I2C_TypeDef *s_i2c  = NULL;
static uint8_t      s_addr = 0U;
static uint8_t      s_col  = 0U;     /* lech cot: SSD1306 = 0, SH1106 = 2 */

/** @brief Gui 1 chuoi lenh toi SSD1306. */
static uint8_t prvCmd(const uint8_t *cmd, uint16_t len)
{
    return hal_i2c_write(s_i2c, s_addr, OLED_CTRL_CMD, cmd, len, OLED_TIMEOUT_MS);
}

uint8_t hal_oled_init(I2C_TypeDef *i2c, uint8_t addr7, uint8_t col_offset)
{
    /* Chuoi khoi tao chuan cho SSD1306 128x64 (theo datasheet) */
    static const uint8_t init_seq[] = {
        0xAE,         /* tat man hinh trong luc cau hinh       */
        0xD5, 0x80,   /* tan so dao dong / chia clock           */
        0xA8, 0x3F,   /* multiplex = 64 dong                    */
        0xD3, 0x00,   /* khong dich man hinh                    */
        0x40,         /* dong bat dau = 0                       */
        0x8D, 0x14,   /* bat bom dien ap noi (charge pump)      */
        0xA1,         /* lat ngang (cot 127 -> SEG0)            */
        0xC8,         /* lat doc (quet tu duoi len)             */
        0xDA, 0x12,   /* cau hinh chan COM                      */
        0x81, 0xCF,   /* do tuong phan                          */
        0xD9, 0xF1,   /* pre-charge                             */
        0xDB, 0x40,   /* muc VCOMH                              */
        0xA4,         /* hien thi theo noi dung RAM             */
        0xA6,         /* binh thuong (khong dao mau)            */
        0xAF          /* bat man hinh                           */
    };

    if (i2c == NULL) {
        return HAL_I2C_ERR_PARAM;
    }
    s_i2c  = i2c;
    s_addr = addr7;
    s_col  = col_offset;

    hal_delay_ms(OLED_POWERUP_MS);
    hal_oled_clear();
    return prvCmd(init_seq, (uint16_t)sizeof(init_seq));
}

void hal_oled_clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

/**
 * @brief Ve 1 ky tu 5x7 vao bo dem (cot thu 6 de trong lam khoang cach).
 *        Ky tu ngoai bang font (0x20..0x7F) duoc thay bang '?'.
 */
static void prvPutc(uint8_t col, uint8_t page, char c)
{
    uint8_t i;
    uint8_t idx;

    if ((uint8_t)c < 0x20U || (uint8_t)c > 0x7FU) {
        c = '?';
    }
    idx = (uint8_t)((uint8_t)c - 0x20U);    /* font bat dau tu ky tu ' ' (0x20) */

    for (i = 0; i < 5U; i++) {
        if ((uint16_t)col + i < OLED_WIDTH) {
            s_fb[page][col + i] = g_font5x7[idx][i];
        }
    }
}

void hal_oled_puts(uint8_t col, uint8_t page, const char *s)
{
    if (page >= OLED_PAGES || s == NULL) {
        return;
    }
    while (*s != '\0' && col <= (OLED_WIDTH - 6U)) {
        prvPutc(col, page, *s++);
        col += 6U;
    }
}

void hal_oled_hline(uint8_t page, uint8_t bit)
{
    uint8_t x;
    if (page >= OLED_PAGES || bit > 7U) {
        return;
    }
    for (x = 0; x < OLED_WIDTH; x++) {
        s_fb[page][x] |= (uint8_t)(1U << bit);
    }
}

uint8_t hal_oled_flush(void)
{
    uint8_t page;
    uint8_t st;

    /* Gui tung page: dat con tro ve dau page roi gui 128 byte.
     * Cach nay chay duoc ca SSD1306 lan SH1106 (man 1.3", can lech cot 2). */
    for (page = 0; page < OLED_PAGES; page++) {
        uint8_t cmd[3];
        cmd[0] = (uint8_t)(0xB0U | page);                              /* chon page    */
        cmd[1] = (uint8_t)(0x00U | (s_col & 0x0FU));     /* cot: 4 bit thap */
        cmd[2] = (uint8_t)(0x10U | (s_col >> 4));        /* cot: 4 bit cao  */

        st = prvCmd(cmd, (uint16_t)sizeof(cmd));
        if (st != HAL_I2C_OK) {
            return st;
        }
        st = hal_i2c_write(s_i2c, s_addr, OLED_CTRL_DATA,
                               s_fb[page], OLED_WIDTH, OLED_TIMEOUT_MS);
        if (st != HAL_I2C_OK) {
            return st;
        }
    }
    return HAL_I2C_OK;
}
