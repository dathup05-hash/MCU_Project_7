/*
 * =============================================================================
 *  File      : bsp_uart.h
 *  Tang      : bsp
 *  Chuc nang : Cong UART debug cua board (chan va toc do xem bsp.h).
 *              Noi module USB-TTL: TX board -> RX module, RX board -> TX module,
 *              GND chung. Mo terminal (Hercules / PuTTY) 115200 8N1.
 * =============================================================================
 */
#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdint.h>

/**
 * @brief  Cau hinh chan TX/RX va khoi tao UART debug. Da duoc goi trong bsp_init().
 * @return 0 = OK, 1 = loi
 */
uint8_t  bsp_uart_init(void);

/** @brief In chuoi, KHONG xuong dong. */
void     bsp_uart_print(const char *s);

/** @brief In chuoi roi xuong dong. */
void     bsp_uart_println(const char *s);

/**
 * @brief  Doc 1 dong nguoi dung go tren terminal (ket thuc bang Enter).
 * @param  buf        [out] Bo dem, luon ket thuc '\0'
 * @param  size       Kich thuoc buf
 * @param  timeout_ms Thoi gian cho toi da
 * @return So ky tu doc duoc, 0 = khong co gi
 * @note   Ham cho ban -> chi goi o task uu tien thap.
 */
uint16_t bsp_uart_read_line(char *buf, uint16_t size, uint32_t timeout_ms);

#endif /* BSP_UART_H */
