/*
 * =============================================================================
 *  File      : hal_ds18b20.c
 *  Tang      : hal (driver thiet bi ngoai)
 *  Chuc nang : Giao thuc 1-Wire + doc nhiet do DS18B20. Xem cach dung trong .h.
 *              Chan DQ duoc truyen vao qua hal_ds18b20_init() -> file nay
 *              KHONG biet board, dung lai duoc cho board khac.
 *
 *  Nguyen ly 1-Wire (1 day DQ, co tro 4.7k keo len 3.3V):
 *    - MCU chi "keo xuong 0" hoac "tha ra" (tro tu keo len 1) -> chan open-drain
 *    - Moi bit truyen trong 1 "khe thoi gian" ~70 us, phan biet 0/1 bang
 *      do dai xung thap -> can delay chinh xac tung micro-giay.
 *
 *  Thoi gian (theo datasheet DS18B20):
 *    Reset : keo 0 trong 480us, tha ra, sau 70us doc: 0 = co cam bien tra loi
 *    Ghi 1 : keo 0 trong 6us, tha 64us
 *    Ghi 0 : keo 0 trong 60us, tha 10us
 *    Doc   : keo 0 3us, tha, sau 10us doc muc chan, cho them 53us
 * =============================================================================
 */
#include <stddef.h>
#include "hal_ds18b20.h"
#include "hal_gpio.h"
#include "hal_delay.h"

/* Chan DQ (luu khi init) */
static GPIO_TypeDef *s_port = NULL;
static uint16_t      s_pin  = 0U;

/* Macro thao tac chan DQ cho code de doc */
#define OW_LOW()        hal_gpio_write(s_port, s_pin, 0U)   /* keo bus xuong 0 */
#define OW_RELEASE()    hal_gpio_write(s_port, s_pin, 1U)   /* tha bus (open-drain) */
#define OW_READ()       hal_gpio_read(s_port, s_pin)        /* doc muc tren bus */

/* Lenh cua DS18B20 */
#define DS_CMD_SKIP_ROM     0xCC    /* bo qua dia chi (chi 1 cam bien tren bus) */
#define DS_CMD_CONVERT_T    0x44    /* bat dau do nhiet do                     */
#define DS_CMD_READ_SCRATCH 0xBE    /* doc 9 byte bo nho tam (scratchpad)      */

/**
 * @brief  Tat ngat tam thoi de khe thoi gian 1-Wire khong bi chen ngang
 *         (SysTick hay ngat nut chen vao giua se lam sai bit).
 * @return Trang thai ngat cu, de prvUnlock() khoi phuc dung.
 * @note   Chi khoa vai chuc us moi lan, khong anh huong FreeRTOS.
 */
static uint32_t prvLock(void)
{
    uint32_t pm = __get_PRIMASK();
    __disable_irq();
    return pm;
}

/** @brief Khoi phuc trang thai ngat da luu boi prvLock(). */
static void prvUnlock(uint32_t pm)
{
    __set_PRIMASK(pm);
}

/**
 * @brief  Gui xung reset va kiem tra cam bien co tra loi (presence pulse) khong.
 * @return 1 = co cam bien, 0 = khong
 */
static uint8_t prvReset(void)
{
    uint8_t  presence;
    uint32_t pm;

    OW_LOW();
    hal_delay_us(480);
    pm = prvLock();
    OW_RELEASE();
    hal_delay_us(70);
    presence = (uint8_t)(OW_READ() == 0U);  /* cam bien keo bus xuong 0 = "co mat" */
    prvUnlock(pm);
    hal_delay_us(410);                      /* cho het khe reset */
    return presence;
}

/** @brief Ghi 1 bit len bus. */
static void prvWriteBit(uint8_t bit)
{
    uint32_t pm = prvLock();
    OW_LOW();
    if (bit) {
        hal_delay_us(6);        /* bit 1: xung thap ngan */
        OW_RELEASE();
        hal_delay_us(64);
    } else {
        hal_delay_us(60);       /* bit 0: xung thap dai */
        OW_RELEASE();
        hal_delay_us(10);
    }
    prvUnlock(pm);
}

/** @brief Doc 1 bit tu bus. */
static uint8_t prvReadBit(void)
{
    uint8_t  bit;
    uint32_t pm = prvLock();
    OW_LOW();
    hal_delay_us(3);
    OW_RELEASE();
    hal_delay_us(10);
    bit = OW_READ();            /* cam bien giu 0 = bit 0, tha ra = bit 1 */
    prvUnlock(pm);
    hal_delay_us(53);
    return bit;
}

/** @brief Ghi 1 byte, bit thap (LSB) truoc. */
static void prvWriteByte(uint8_t v)
{
    uint8_t i;
    for (i = 0; i < 8U; i++) {
        prvWriteBit(v & 0x01U);
        v >>= 1;
    }
}

/** @brief Doc 1 byte, bit thap (LSB) truoc. */
static uint8_t prvReadByte(void)
{
    uint8_t i, v = 0;
    for (i = 0; i < 8U; i++) {
        if (prvReadBit()) {
            v |= (uint8_t)(1U << i);
        }
    }
    return v;
}

/**
 * @brief  Tinh CRC8 kieu Dallas/Maxim (da thuc 0x8C) de kiem tra du lieu.
 * @return Gia tri CRC; dung neu bang byte CRC cam bien gui kem.
 */
static uint8_t prvCrc8(const uint8_t *p, uint8_t len)
{
    uint8_t crc = 0, i, b;
    for (i = 0; i < len; i++) {
        uint8_t in = p[i];
        for (b = 0; b < 8U; b++) {
            uint8_t mix = (uint8_t)((crc ^ in) & 0x01U);
            crc >>= 1;
            if (mix) {
                crc ^= 0x8CU;
            }
            in >>= 1;
        }
    }
    return crc;
}

uint8_t hal_ds18b20_init(GPIO_TypeDef *port, uint16_t pin)
{
    if (hal_gpio_config(port, pin, HAL_GPIO_OUT_OD) != 0U) {
        return 1U;
    }
    s_port = port;
    s_pin  = pin;
    OW_RELEASE();
    return 0U;
}

uint8_t hal_ds18b20_start_conversion(void)
{
    if (s_port == NULL || !prvReset()) {
        return 1U;
    }
    prvWriteByte(DS_CMD_SKIP_ROM);
    prvWriteByte(DS_CMD_CONVERT_T);
    return 0U;
}

uint8_t hal_ds18b20_read_temp(int16_t *temp_x10)
{
    uint8_t sp[9];      /* scratchpad: [0..1] nhiet do, [4] config, [8] CRC */
    uint8_t i;
    int16_t raw;

    if (temp_x10 == NULL || s_port == NULL || !prvReset()) {
        return 1U;
    }
    prvWriteByte(DS_CMD_SKIP_ROM);
    prvWriteByte(DS_CMD_READ_SCRATCH);
    for (i = 0; i < 9U; i++) {
        sp[i] = prvReadByte();
    }

    /* Kiem tra du lieu:
     * - CRC phai khop
     * - Byte config co bit 0..4 luon = 1 -> loai truong hop bus bi treo o 0
     *   (khi do doc ra toan 0, CRC cua toan 0 cung = 0 nen CRC van "dung") */
    if (prvCrc8(sp, 8) != sp[8] || (sp[4] & 0x9FU) != 0x1FU) {
        return 1U;
    }

    /* Gia tri tho 16 bit co dau, 1 don vi = 1/16 do C -> doi sang x10 */
    raw = (int16_t)(((uint16_t)sp[1] << 8) | sp[0]);
    *temp_x10 = (int16_t)(((int32_t)raw * 10) / 16);
    return 0U;
}
