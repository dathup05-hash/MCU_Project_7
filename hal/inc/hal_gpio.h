/*
 * =============================================================================
 *  File      : hal_gpio.h
 *  Tang      : hal
 *  Chuc nang : Dieu khien chan GPIO, boc thu vien SPL (stm32f10x_gpio.c).
 *              Gom 3 nhom ham:
 *                1. Cau hinh : hal_gpio_config()
 *                2. Bat tat  : hal_gpio_write(), hal_gpio_toggle()
 *                3. Doc      : hal_gpio_read()
 *
 *  Tham so chung:
 *    port : GPIOA, GPIOB, GPIOC, GPIOD
 *    pin  : mask kieu SPL, vd GPIO_Pin_13 (= 1 << 13).
 *           Ghep nhieu chan bang dau |, vd GPIO_Pin_6 | GPIO_Pin_7
 *           (rieng hal_gpio_read chi dung 1 chan).
 *
 *  Quy uoc gia tri tra ve: 0 = thanh cong, 1 = tham so sai.
 * =============================================================================
 */
#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include <stdint.h>
#include "stm32f10x.h"

/* -----------------------------------------------------------------------------
 *  Che do chan (tham so `mode` cua hal_gpio_config)
 * -------------------------------------------------------------------------- */
#define HAL_GPIO_OUT_PP         0U  /* output push-pull: chu dong ra 0 / 1 (LED, coi, relay)     */
#define HAL_GPIO_OUT_OD         1U  /* output open-drain: chi keo 0, tha ra = noi (1-Wire)       */
#define HAL_GPIO_IN_FLOAT       2U  /* input tha noi (UART RX, khi da co tro keo ngoai)          */
#define HAL_GPIO_IN_PULLUP      3U  /* input, tro keo len noi: de ho = 1, nut noi GND (3 nut)   */
#define HAL_GPIO_IN_PULLDOWN    4U  /* input, tro keo xuong noi: de ho = 0                       */
#define HAL_GPIO_AF_PP          5U  /* chuc nang phu push-pull (UART TX, TIMER PWM)              */
#define HAL_GPIO_AF_OD          6U  /* chuc nang phu open-drain (bat buoc cho I2C SCL/SDA)       */
#define HAL_GPIO_MODE_COUNT     7U  /* so che do hop le, dung de kiem tra tham so                */

/**
 * @brief  Cau hinh che do cho 1 hoac nhieu chan cua 1 port.
 *         Tu bat clock cua port (va clock AFIO neu la che do AF).
 * @param  port GPIOA..GPIOD
 * @param  pin  Mask chan, vd GPIO_Pin_13 hoac GPIO_Pin_6 | GPIO_Pin_7
 * @param  mode Mot trong cac HAL_GPIO_xxx o tren
 * @return 0 = OK, 1 = tham so sai (port khong ho tro, pin = 0, mode sai)
 * @note   Goi luc khoi dong, truoc vTaskStartScheduler(). Khong goi trong ngat.
 */
uint8_t hal_gpio_config(GPIO_TypeDef *port, uint16_t pin, uint8_t mode);

/**
 * @brief  Xuat muc logic ra chan (bat / tat).
 * @param  level 0 = muc thap (0V), khac 0 = muc cao (3.3V)
 * @note   Nguyen tu (ghi BSRR/BRR), an toan khi nhieu task cung dung 1 port.
 */
void    hal_gpio_write(GPIO_TypeDef *port, uint16_t pin, uint8_t level);

/**
 * @brief  Dao muc output hien tai cua chan (dang 1 -> 0, dang 0 -> 1).
 * @note   Dung duoc cho nhieu chan cung luc, moi chan dao rieng.
 */
void    hal_gpio_toggle(GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief  Doc muc logic dang co tren chan.
 * @param  pin CHI 1 chan (vd GPIO_Pin_13)
 * @return 1 = muc cao, 0 = muc thap
 * @note   Nut noi GND + HAL_GPIO_IN_PULLUP: khong bam = 1, dang bam = 0.
 */
uint8_t hal_gpio_read(GPIO_TypeDef *port, uint16_t pin);

/* -----------------------------------------------------------------------------
 *  NGAT NGOAI (EXTI)
 *  STM32F1 co 16 line EXTI, line N dung cho chan so N (cua 1 port bat ky).
 *  Vd PA2 -> line 2, PB2 cung la line 2 -> moi so chan chi dung duoc 1 port.
 * -------------------------------------------------------------------------- */
#define HAL_EXTI_RISING         0U  /* canh len  (0 -> 1)                      */
#define HAL_EXTI_FALLING        1U  /* canh xuong (1 -> 0): luc bam nut keo len */
#define HAL_EXTI_BOTH           2U  /* ca hai canh                             */

/** Ham goi lai khi co ngat. Chay TRONG NGAT -> chi dung API FreeRTOS ...FromISR(). */
typedef void (*hal_exti_cb_t)(uint16_t pin);

/**
 * @brief  Bat ngat ngoai cho 1 chan (chan phai cau hinh input truoc).
 * @param  port     GPIOA..GPIOD
 * @param  pin      CHI 1 chan, vd GPIO_Pin_2
 * @param  trigger  HAL_EXTI_RISING / HAL_EXTI_FALLING / HAL_EXTI_BOTH
 * @param  irq_prio Uu tien ngat 0..15 (so lon = uu tien thap).
 *                  PHAI >= 5 thi moi duoc goi API FreeRTOS ...FromISR() trong ngat.
 * @param  cb       Ham goi khi co ngat
 * @return 0 = OK, 1 = tham so sai
 */
uint8_t hal_exti_config(GPIO_TypeDef *port, uint16_t pin, uint8_t trigger,
                        uint8_t irq_prio, hal_exti_cb_t cb);

#endif /* HAL_GPIO_H */
