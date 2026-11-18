/*
 * =============================================================================
 *  File      : hal_uart.c
 *  Tang      : hal
 *  Chuc nang : UART polling cho debug, boc thu vien SPL (stm32f10x_usart.c).
 *              Xem mo ta ham public trong hal_uart.h.
 *
 *  Gui 1 byte: cho co TXE (thanh ghi gui trong) -> ghi DR.
 *  Nhan 1 byte: cho co RXNE (co du lieu moi)   -> doc DR.
 *  Moi vong cho deu co gioi han thoi gian -> khong treo khi UART chua init
 *  hoac chua cam day.
 * =============================================================================
 */
#include "hal_uart.h"
#include "hal_delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define UART_COUNT          3U
#define UART_BAD_IDX        0xFFU
#define UART_TX_TIMEOUT_US  2000UL      /* 1 byte o 9600 baud mat ~1 ms -> du du */
#define UART_LOCK_MS        100UL       /* cho Mutex toi da; qua thi bo dong in do */
#define UART_MAX_TIMEOUT_MS 50000UL     /* tranh tran phep nhan timeout_ms * 1000 */

static SemaphoreHandle_t s_mutex[UART_COUNT];
static uint8_t           s_ready[UART_COUNT];       /* 1 = da init */

/* ========================================================================== */
/*                                 PRIVATE                                    */
/* ========================================================================== */

/** @brief Doi con tro USART sang chi so mang. @return 0..2 hoac UART_BAD_IDX. */
static uint8_t prvIdx(USART_TypeDef *uart)
{
    if (uart == USART1) return 0U;
    if (uart == USART2) return 1U;
    if (uart == USART3) return 2U;
    return UART_BAD_IDX;
}

/** @brief Gui 1 byte, cho TXE co gioi han thoi gian. */
static void prvTx(USART_TypeDef *uart, uint8_t b)
{
    uint32_t left_us = UART_TX_TIMEOUT_US;

    while (USART_GetFlagStatus(uart, USART_FLAG_TXE) == RESET) {
        if (left_us == 0U) {
            return;     /* phan cung khong phan hoi -> bo byte nay, khong treo */
        }
        left_us--;
        hal_delay_us(1U);
    }
    USART_SendData(uart, b);
}

/** @brief Gui ca chuoi, khong khoa (ham goi tu lo khoa). */
static void prvTxStr(USART_TypeDef *uart, const char *s)
{
    while (*s != '\0') {
        prvTx(uart, (uint8_t)*s);
        s++;
    }
}

/**
 * @brief  Khoa phan gui cua UART. Truoc khi scheduler chay chi co 1 luong nen bo qua.
 * @return 1 = duoc phep gui, 0 = task khac giu qua lau -> bo qua lan in nay
 */
static uint8_t prvLock(uint8_t idx, uint8_t *locked)
{
    *locked = 0U;
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return 1U;
    }
    if (xSemaphoreTake(s_mutex[idx], pdMS_TO_TICKS(UART_LOCK_MS)) != pdTRUE) {
        return 0U;
    }
    *locked = 1U;
    return 1U;
}

/** @brief Mo khoa neu da khoa. */
static void prvUnlock(uint8_t idx, uint8_t locked)
{
    if (locked) {
        xSemaphoreGive(s_mutex[idx]);
    }
}

/** @brief Gui chuoi co khoa, `eol` = 1 thi them "\r\n". */
static void prvWriteLine(USART_TypeDef *uart, const char *s, uint8_t eol)
{
    uint8_t idx = prvIdx(uart);
    uint8_t locked;

    if (idx == UART_BAD_IDX || !s_ready[idx] || s == NULL) {
        return;
    }
    if (!prvLock(idx, &locked)) {
        return;
    }
    prvTxStr(uart, s);
    if (eol) {
        prvTxStr(uart, "\r\n");
    }
    prvUnlock(idx, locked);
}

/* ========================================================================== */
/*                                  PUBLIC                                    */
/* ========================================================================== */

uint8_t hal_uart_init(USART_TypeDef *uart, uint32_t baud)
{
    USART_InitTypeDef ui;
    uint8_t idx = prvIdx(uart);

    if (idx == UART_BAD_IDX || baud == 0U) {
        return 1U;
    }

    /* USART1 nam tren bus APB2, USART2/3 nam tren APB1 */
    if (uart == USART1) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    } else if (uart == USART2) {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    } else {
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    }

    USART_StructInit(&ui);
    ui.USART_BaudRate            = baud;
    ui.USART_WordLength          = USART_WordLength_8b;
    ui.USART_StopBits            = USART_StopBits_1;
    ui.USART_Parity              = USART_Parity_No;
    ui.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    ui.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(uart, &ui);
    USART_Cmd(uart, ENABLE);

    if (s_mutex[idx] == NULL) {
        s_mutex[idx] = xSemaphoreCreateMutex();
        if (s_mutex[idx] == NULL) {
            return 1U;
        }
    }
    s_ready[idx] = 1U;
    return 0U;
}

void hal_uart_putc(USART_TypeDef *uart, char c)
{
    uint8_t idx = prvIdx(uart);

    if (idx == UART_BAD_IDX || !s_ready[idx]) {
        return;
    }
    prvTx(uart, (uint8_t)c);    /* 1 ky tu: khong can khoa */
}

void hal_uart_puts(USART_TypeDef *uart, const char *s)
{
    prvWriteLine(uart, s, 0U);
}

void hal_uart_println(USART_TypeDef *uart, const char *s)
{
    prvWriteLine(uart, s, 1U);
}

uint16_t hal_uart_gets(USART_TypeDef *uart, char *buf, uint16_t size, uint32_t timeout_ms)
{
    uint8_t  idx = prvIdx(uart);
    uint16_t n = 0U;
    uint32_t left_us;   /* thoi gian cho con lai */
    char     c;

    if (buf == NULL || size < 2U) {
        return 0U;
    }
    buf[0] = '\0';
    if (idx == UART_BAD_IDX || !s_ready[idx]) {
        return 0U;
    }
    if (timeout_ms > UART_MAX_TIMEOUT_MS) {
        timeout_ms = UART_MAX_TIMEOUT_MS;
    }

    left_us = timeout_ms * 1000UL;
    while (n < (uint16_t)(size - 1U)) {
        if (USART_GetFlagStatus(uart, USART_FLAG_RXNE) == SET) {
            c = (char)USART_ReceiveData(uart);  /* doc DR xoa luon co RXNE va ORE */
            if (c == '\r' || c == '\n') {
                if (n > 0U) {
                    break;                      /* het dong */
                }
                continue;                       /* bo '\n' thua cua "\r\n" truoc do */
            }
            buf[n] = c;
            n++;
        } else if (USART_GetFlagStatus(uart, USART_FLAG_ORE) == SET) {
            (void)USART_ReceiveData(uart);      /* tran bo dem: bo byte loi, xoa co */
        } else if (left_us == 0U) {
            break;                              /* het gio */
        } else {
            left_us--;
            hal_delay_us(1U);
        }
    }
    buf[n] = '\0';
    return n;
}
