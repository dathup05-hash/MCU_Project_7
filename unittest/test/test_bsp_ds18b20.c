/*
 * =============================================================================
 *  File      : test_bsp_ds18b20.c
 *  Kieu test : Unity + CMock (mock tang hal)
 *  Doi tuong : bsp/bsp_ds18b20.c
 *
 *  Y tuong: thay hal_gpio va hal_delay bang MOCK, roi dung "callback" cua CMock
 *  de MO PHONG 1 con DS18B20 tren day 1-Wire:
 *    - hal_gpio_read  : tra ve bit ma cam bien "gui len"
 *    - hal_delay_us   : ghi lai do dai tung khe thoi gian -> giai ma duoc
 *                       chuoi lenh ma driver da gui xuong (reset, bit 0, bit 1)
 * =============================================================================
 */
#include <string.h>
#include "unity.h"
#include "bsp_ds18b20.h"
#include "mock_hal_gpio.h"
#include "mock_hal_delay.h"

/* --------------------------- Cam bien gia lap --------------------------- */

static uint8_t  s_present;          /* 1 = co cam bien tren bus               */
static uint8_t  s_scratch[9];       /* 9 byte cam bien se tra ve              */
static uint8_t  s_stuck_low;        /* 1 = bus bi chap xuong 0                */

static uint32_t s_delays[2000];     /* cac khoang delay driver da goi (us)    */
static uint32_t s_ndelay;

/** CRC8 Dallas tham chieu (de tao du lieu hop le cho test) */
static uint8_t crc8(const uint8_t *p, uint8_t len)
{
    uint8_t crc = 0;
    while (len--) {
        uint8_t in = *p++, b;
        for (b = 0; b < 8; b++) {
            uint8_t mix = (crc ^ in) & 1U;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            in >>= 1;
        }
    }
    return crc;
}

/**
 * Callback thay cho hal_gpio_read(). Driver doc bus o 2 noi:
 *   - lan doc dau tien (n = 0): xung "presence" sau reset (0 = co cam bien)
 *   - cac lan sau: tung bit du lieu, LSB truoc
 */
static uint8_t fake_bus_read(GPIO_TypeDef *port, uint16_t pin, int n)
{
    int bit;
    (void)port; (void)pin;
    if (s_stuck_low) return 0;
    if (n == 0)      return s_present ? 0 : 1;
    bit = n - 1;
    if (bit >= 72)   return 1;
    return (uint8_t)((s_scratch[bit / 8] >> (bit % 8)) & 1U);
}

/** Callback thay cho hal_delay_us(): ghi lai do dai khe thoi gian */
static void fake_delay(uint32_t us, int n)
{
    (void)n;
    if (s_ndelay < (sizeof(s_delays) / sizeof(s_delays[0]))) {
        s_delays[s_ndelay++] = us;
    }
}

/**
 * Giai ma chuoi delay thanh cac byte driver da GHI xuong bus.
 *   reset = 480,70,410 | ghi 1 = 6,64 | ghi 0 = 60,10 | doc bit = 3,10,53
 * @return so byte giai ma duoc; *resets = so lan reset
 */
static int decode_written(uint8_t *out, int max, int *resets)
{
    uint32_t i = 0;
    int nbits = 0, nbytes = 0;
    uint8_t cur = 0;
    *resets = 0;
    while (i < s_ndelay) {
        if (s_delays[i] == 480) { (*resets)++; i += 3; continue; }
        if (s_delays[i] == 3)   { i += 3; continue; }           /* khe doc */
        if (s_delays[i] == 6 || s_delays[i] == 60) {
            if (s_delays[i] == 6) cur |= (uint8_t)(1U << nbits);
            nbits++; i += 2;
            if (nbits == 8) {
                if (nbytes < max) out[nbytes] = cur;
                nbytes++; nbits = 0; cur = 0;
            }
            continue;
        }
        i++;
    }
    return nbytes;
}

/** Nap scratchpad voi gia tri tho cua nhiet do, CRC dung */
static void load_scratch(int16_t raw)
{
    s_scratch[0] = (uint8_t)(raw & 0xFF);
    s_scratch[1] = (uint8_t)((uint16_t)raw >> 8);
    s_scratch[2] = 0x4B;        /* TH  */
    s_scratch[3] = 0x46;        /* TL  */
    s_scratch[4] = 0x7F;        /* config: 12 bit */
    s_scratch[5] = 0xFF;
    s_scratch[6] = 0x0C;
    s_scratch[7] = 0x10;
    s_scratch[8] = crc8(s_scratch, 8);
}

void setUp(void)
{
    s_present = 1;
    s_stuck_low = 0;
    s_ndelay = 0;
    memset(s_scratch, 0, sizeof(s_scratch));

    hal_gpio_write_Ignore();
    hal_gpio_config_Ignore();
    hal_gpio_read_Stub(fake_bus_read);
    hal_delay_us_Stub(fake_delay);
}

void tearDown(void)
{
}

/* ================================ Test =================================== */

void test_start_conversion_fails_when_no_sensor(void)
{
    s_present = 0;
    TEST_ASSERT_EQUAL(HAL_ERR, bsp_ds18b20_start_conversion());
}

void test_start_conversion_sends_reset_skiprom_convert(void)
{
    uint8_t bytes[4];
    int resets;

    TEST_ASSERT_EQUAL(HAL_OK, bsp_ds18b20_start_conversion());

    TEST_ASSERT_EQUAL_INT(2, decode_written(bytes, 4, &resets));
    TEST_ASSERT_EQUAL_INT(1, resets);
    TEST_ASSERT_EQUAL_HEX8(0xCC, bytes[0]);     /* Skip ROM  */
    TEST_ASSERT_EQUAL_HEX8(0x44, bytes[1]);     /* Convert T */
}

void test_read_sends_skiprom_then_read_scratchpad(void)
{
    uint8_t bytes[4];
    int resets;
    int16_t t;

    load_scratch(0x0191);
    TEST_ASSERT_EQUAL(HAL_OK, bsp_ds18b20_read_temp(&t));

    TEST_ASSERT_EQUAL_INT(2, decode_written(bytes, 4, &resets));
    TEST_ASSERT_EQUAL_HEX8(0xCC, bytes[0]);     /* Skip ROM        */
    TEST_ASSERT_EQUAL_HEX8(0xBE, bytes[1]);     /* Read Scratchpad */
}

void test_read_positive_temperature(void)
{
    int16_t t = 0;
    load_scratch(0x0191);       /* +25.0625 C (vi du trong datasheet) */
    TEST_ASSERT_EQUAL(HAL_OK, bsp_ds18b20_read_temp(&t));
    TEST_ASSERT_EQUAL_INT16(250, t);
}

void test_read_negative_temperature(void)
{
    int16_t t = 0;
    load_scratch((int16_t)0xFF5E);   /* -10.125 C (vi du trong datasheet) */
    TEST_ASSERT_EQUAL(HAL_OK, bsp_ds18b20_read_temp(&t));
    TEST_ASSERT_EQUAL_INT16(-101, t);
}

void test_read_85_degrees(void)
{
    int16_t t = 0;
    load_scratch(0x0550);       /* +85 C */
    TEST_ASSERT_EQUAL(HAL_OK, bsp_ds18b20_read_temp(&t));
    TEST_ASSERT_EQUAL_INT16(850, t);
}

void test_read_fails_on_bad_crc(void)
{
    int16_t t = 123;
    load_scratch(0x0191);
    s_scratch[8] ^= 0x01;       /* lam hong 1 bit CRC */
    TEST_ASSERT_EQUAL(HAL_ERR, bsp_ds18b20_read_temp(&t));
    TEST_ASSERT_EQUAL_INT16(123, t);        /* khong ghi de gia tri cu */
}

void test_read_fails_when_no_sensor(void)
{
    int16_t t;
    s_present = 0;
    TEST_ASSERT_EQUAL(HAL_ERR, bsp_ds18b20_read_temp(&t));
}

void test_read_fails_when_bus_stuck_low(void)
{
    /* Bus chap 0: doc ra toan 0, CRC cua toan 0 cung = 0 (trung khop!)
     * -> driver phai dua vao byte config de phat hien loi. */
    int16_t t;
    s_stuck_low = 1;
    TEST_ASSERT_EQUAL(HAL_ERR, bsp_ds18b20_read_temp(&t));
}
