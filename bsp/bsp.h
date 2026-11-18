/*
 * =============================================================================
 *  File      : bsp.h
 *  Tang      : bsp (Board Support Package)
 *  Chuc nang : BANG CHAN cua board + ham dieu khien linh kien gan tren board.
 *              Board: STM32F103C8T6 (Blue Pill), thach anh 8 MHz, chay 72 MHz.
 *              Doi chan cam / doi linh kien thi CHI sua file nay.
 *
 *  Khoi             Chan        Ghi chu
 *  ---------------  ----------  ---------------------------------------------
 *  LED canh bao     PC13        LED tren board, muc THAP = sang
 *  Coi (buzzer)     PB12        Muc cao = keu (qua transistor)
 *  Relay quat       PA0         Module relay (IN), xem BSP_RELAY_ACTIVE_LOW
 *  DS18B20 (DQ)     PA1         1-Wire, tro keo len 4.7k ve 3.3V (bat buoc)
 *  OLED SSD1306     PB6 / PB7   I2C1 SCL / SDA, dia chi 0x3C
 *  Nut UP           PA2         Tang nguong. Noi xuong GND, tro keo len noi
 *  Nut DOWN         PA3         Giam nguong. Noi xuong GND, tro keo len noi
 *  Nut RESET        PA4         Nguong ve mac dinh. Noi xuong GND, tro keo len noi
 *  UART debug       PA9 / PA10  USART1 TX / RX, 115200 8N1 -> module USB-TTL
 *
 *  Cac file trong tang bsp:
 *    bsp.h / bsp.c          : bang chan (file nay), bsp_init(), LED, coi
 *    bsp_button.h / .c      : 3 nut nhan
 *    bsp_relay.h / .c       : relay quat
 *    bsp_uart.h / .c        : UART debug
 *    bsp_ds18b20.h / .c     : gan cam bien nhiet do vao chan (driver: hal_ds18b20)
 *    bsp_oled.h / .c        : gan man hinh OLED vao bus I2C (driver: hal_oled)
 *
 *  Ham trong bsp dat ten theo LINH KIEN (bsp_relay_set, bsp_buzzer_set...),
 *  tang app goi cac ham nay ma khong can biet chan nao, muc logic nao.
 * =============================================================================
 */
#ifndef BSP_H
#define BSP_H

#include <stdint.h>
#include "stm32f10x.h"

/* ----------------------------- LED canh bao -------------------------------- */
#define BSP_LED_PORT            GPIOC
#define BSP_LED_PIN             GPIO_Pin_13
#define BSP_LED_ACTIVE_LOW      1U      /* LED Blue Pill: ghi 0 thi sang */

/* --------------------------------- Coi ------------------------------------- */
#define BSP_BUZZER_PORT         GPIOB
#define BSP_BUZZER_PIN          GPIO_Pin_12

/* ------------------------------ Relay quat --------------------------------- */
#define BSP_RELAY_PORT          GPIOA
#define BSP_RELAY_PIN           GPIO_Pin_0
#define BSP_RELAY_ACTIVE_LOW    0U      /* jumper H tren module: muc CAO = dong relay.
                                           Neu dat jumper L thi doi thanh 1U.        */

/* -------------------------------- DS18B20 ---------------------------------- */
#define BSP_OW_PORT             GPIOA
#define BSP_OW_PIN              GPIO_Pin_1

/* ---------------------------------- OLED ----------------------------------- */
#define BSP_OLED_I2C            I2C1
#define BSP_OLED_I2C_PORT       GPIOB
#define BSP_OLED_SCL_PIN        GPIO_Pin_6
#define BSP_OLED_SDA_PIN        GPIO_Pin_7
#define BSP_OLED_I2C_SPEED      400000UL
#define BSP_OLED_ADDR           0x3CU
#define BSP_OLED_COL_OFFSET     0U      /* SSD1306 0.96" = 0, man 1.3" SH1106 = 2 */

/* -------------------------------- Nut nhan --------------------------------- */
#define BSP_BTN_PORT            GPIOA
#define BSP_BTN_UP_PIN          GPIO_Pin_2
#define BSP_BTN_DOWN_PIN        GPIO_Pin_3
#define BSP_BTN_RESET_PIN       GPIO_Pin_4
#define BSP_BTN_IRQ_PRIO        6U      /* uu tien ngat nut: PHAI >= 5 (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) */

/* ------------------------------- UART debug -------------------------------- */
#define BSP_DEBUG_UART          USART1
#define BSP_DEBUG_PORT          GPIOA
#define BSP_DEBUG_TX_PIN        GPIO_Pin_9
#define BSP_DEBUG_RX_PIN        GPIO_Pin_10
#define BSP_DEBUG_BAUD          115200UL

/* Cong tac UART debug:
 *   0 = TAT: khong khoi tao UART, moi lenh in (bsp_uart_print/println) bi bo qua
 *   1 = BAT: in log ra PA9 de xem tren Hercules (115200 8N1)  */
#define BSP_DEBUG_UART_ENABLE   1U

/**
 * @brief  Khoi tao toan bo board, dua moi thiet bi ve trang thai AN TOAN
 *         (LED tat, coi tat, relay ngat). Goi DAU TIEN trong main().
 *         Ben trong goi init cua tung module: button, relay, uart, ds18b20, I2C.
 * @return 0 = OK, 1 = co bus khong khoi tao duoc (I2C / UART)
 * @note   Chua khoi tao OLED (goi bsp_oled_init() rieng, trong task hien thi).
 */
uint8_t bsp_init(void);

/** @brief Bat/tat LED canh bao. @param on 1 = sang, 0 = tat */
void    bsp_led_set(uint8_t on);

/** @brief Bat/tat coi. @param on 1 = keu, 0 = im */
void    bsp_buzzer_set(uint8_t on);

#endif /* BSP_H */
