/*
 * =============================================================================
 *  File      : bsp_button.h
 *  Tang      : bsp
 *  Chuc nang : 3 nut nhan UP / DOWN / RESET (chan xem bsp.h).
 *              Nut noi xuong GND, dung tro keo len noi: khong bam = 1, bam = 0.
 *              Bam nut -> canh xuong -> ngat EXTI -> goi ham cua tang app.
 * =============================================================================
 */
#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

#include <stdint.h>

/* ID nut */
#define BSP_BTN_UP              0U      /* tang nguong            */
#define BSP_BTN_DOWN            1U      /* giam nguong            */
#define BSP_BTN_RESET           2U      /* nguong ve mac dinh     */
#define BSP_BTN_COUNT           3U

/** Ham goi khi co nut bam. Chay TRONG NGAT -> chi dung API FreeRTOS ...FromISR(). */
typedef void (*bsp_button_cb_t)(uint8_t btn);

/** @brief Cau hinh 3 chan nut la input keo len. Da duoc goi trong bsp_init(). */
void    bsp_button_init(void);

/**
 * @brief  Bat ngat canh xuong cho 3 nut va dang ky ham goi lai.
 * @note   Goi trong task (InputTask), sau khi hang doi nut da duoc tao.
 */
void    bsp_button_irq_init(bsp_button_cb_t cb);

/**
 * @brief  Doc trang thai nut ngay luc goi.
 * @return 1 = dang bam, 0 = khong bam (hoac btn sai)
 */
uint8_t bsp_button_pressed(uint8_t btn);

#endif /* BSP_BUTTON_H */
