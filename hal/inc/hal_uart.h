/*
 * =============================================================================
 *  File      : hal_uart.h
 *  Tang      : hal
 *  Chuc nang : UART dang polling, gui/doc don gian - muc dich chinh la DEBUG.
 *                - Cau hinh : hal_uart_init()
 *                - Gui      : hal_uart_putc()    1 ky tu
 *                             hal_uart_puts()    1 chuoi, KHONG xuong dong
 *                             hal_uart_println() 1 chuoi, CO xuong dong ("\r\n")
 *                - Doc      : hal_uart_gets()    1 chuoi, den khi gap Enter / day / het gio
 *
 *  Khung truyen: 8 bit du lieu, khong parity, 1 stop bit (8N1) - mac dinh cua
 *  cac phan mem terminal (Hercules, PuTTY, Arduino Serial Monitor).
 *
 *  Chan (cau hinh bang hal_gpio_config TRUOC khi init):
 *    USART1: TX = PA9  (HAL_GPIO_AF_PP), RX = PA10 (HAL_GPIO_IN_PULLUP)
 *    USART2: TX = PA2  (HAL_GPIO_AF_PP), RX = PA3  (HAL_GPIO_IN_PULLUP)
 *    USART3: TX = PB10 (HAL_GPIO_AF_PP), RX = PB11 (HAL_GPIO_IN_PULLUP)
 *  Noi voi module USB-TTL: TX chip -> RX module, RX chip -> TX module, GND chung.
 *
 *  Dung voi FreeRTOS:
 *    - puts/println co Mutex: nhieu task cung in thi tung dong khong bi tron chu.
 *    - gets la ham CHO BAN (CPU quay vong) -> chi goi o task uu tien thap,
 *      timeout ngan. Khong goi bat ky ham nao trong file nay tu ngat.
 * =============================================================================
 */
#ifndef HAL_UART_H
#define HAL_UART_H

#include <stdint.h>
#include "stm32f10x.h"

/**
 * @brief  Khoi tao UART 8N1, bat ca TX va RX. Tao Mutex cho phan gui.
 * @param  uart USART1, USART2 hoac USART3
 * @param  baud Toc do, vd 9600 hoac 115200
 * @return 0 = OK, 1 = tham so sai / het heap
 * @note   Goi trong main() truoc vTaskStartScheduler().
 */
uint8_t  hal_uart_init(USART_TypeDef *uart, uint32_t baud);

/** @brief Gui 1 ky tu (cho den khi thanh ghi gui trong). */
void     hal_uart_putc(USART_TypeDef *uart, char c);

/** @brief Gui chuoi ket thuc '\0', khong them xuong dong. */
void     hal_uart_puts(USART_TypeDef *uart, const char *s);

/** @brief Gui chuoi ket thuc '\0' roi xuong dong ("\r\n"). */
void     hal_uart_println(USART_TypeDef *uart, const char *s);

/**
 * @brief  Doc 1 chuoi ky tu.
 *         Dung khi: gap '\r' hoac '\n' (nguoi dung bam Enter), HOAC bo dem day,
 *         HOAC het timeout_ms. Ky tu '\r' / '\n' khong duoc luu vao buf.
 * @param  buf        [out] Bo dem nhan, luon ket thuc bang '\0'
 * @param  size       Kich thuoc buf (byte), toi thieu 2
 * @param  timeout_ms Thoi gian cho toi da (toi da 50000 ms)
 * @return So ky tu da doc (khong tinh '\0'). 0 = khong nhan duoc gi.
 */
uint16_t hal_uart_gets(USART_TypeDef *uart, char *buf, uint16_t size, uint32_t timeout_ms);

#endif /* HAL_UART_H */
