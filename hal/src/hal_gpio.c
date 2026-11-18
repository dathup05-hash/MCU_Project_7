/*
 * =============================================================================
 *  File      : hal_gpio.c
 *  Tang      : hal
 *  Chuc nang : GPIO co ban (cau hinh / bat tat / doc), boc thu vien SPL.
 *              Xem mo ta tung ham trong hal_gpio.h.
 *
 *  Ghi chu:
 *    - Chi kiem tra tham so trong hal_gpio_config (goi 1 lan luc khoi dong).
 *      write/read/toggle goi rat nhieu lan nen khong kiem tra -> nhanh.
 *    - Che do pull-up/pull-down: SPL GPIO_Init tu ghi ODR de chon keo len/xuong.
 *    - PA13, PA14 (SWD) va PB3, PB4, PA15 (JTAG) mac dinh dang dung cho nap/debug.
 *      Khong dung cac chan nay lam GPIO neu chua tat JTAG.
 * =============================================================================
 */
#include "hal_gpio.h"

/**
 * @brief  Bat clock cho port GPIO (STM32 tat clock ngoai vi mac dinh de tiet kiem dien).
 * @return 0 = OK, 1 = port khong ho tro
 */
static uint8_t prvClockEnable(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    } else if (port == GPIOB) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if (port == GPIOC) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    } else if (port == GPIOD) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    } else {
        return 1U;
    }
    return 0U;
}

/** @brief Doi che do HAL_GPIO_xxx sang hang so GPIO_Mode_xxx cua SPL. */
static GPIOMode_TypeDef prvSplMode(uint8_t mode)
{
    switch (mode) {
    case HAL_GPIO_OUT_PP:      return GPIO_Mode_Out_PP;
    case HAL_GPIO_OUT_OD:      return GPIO_Mode_Out_OD;
    case HAL_GPIO_IN_PULLUP:   return GPIO_Mode_IPU;
    case HAL_GPIO_IN_PULLDOWN: return GPIO_Mode_IPD;
    case HAL_GPIO_AF_PP:       return GPIO_Mode_AF_PP;
    case HAL_GPIO_AF_OD:       return GPIO_Mode_AF_OD;
    default:                   return GPIO_Mode_IN_FLOATING;
    }
}

uint8_t hal_gpio_config(GPIO_TypeDef *port, uint16_t pin, uint8_t mode)
{
    GPIO_InitTypeDef gi;

    if (pin == 0U || mode >= HAL_GPIO_MODE_COUNT) {
        return 1U;
    }
    if (prvClockEnable(port) != 0U) {
        return 1U;
    }

    /* Chan chuc nang phu (I2C, UART, TIMER) can them clock khoi AFIO */
    if (mode == HAL_GPIO_AF_PP || mode == HAL_GPIO_AF_OD) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    }

    gi.GPIO_Pin   = pin;
    gi.GPIO_Mode  = prvSplMode(mode);
    gi.GPIO_Speed = GPIO_Speed_50MHz;   /* chi co tac dung voi output, input bo qua */
    GPIO_Init(port, &gi);

    return 0U;
}

void hal_gpio_write(GPIO_TypeDef *port, uint16_t pin, uint8_t level)
{
    if (level != 0U) {
        GPIO_SetBits(port, pin);        /* ghi BSRR: chi chan trong mask len 1 */
    } else {
        GPIO_ResetBits(port, pin);      /* ghi BRR : chi chan trong mask ve 0 */
    }
}

void hal_gpio_toggle(GPIO_TypeDef *port, uint16_t pin)
{
    uint16_t odr = GPIO_ReadOutputData(port);   /* muc output hien tai cua ca port */

    /* Chan dang 1 -> ve 0, chan dang 0 -> len 1 (xu ly dung ca khi pin co nhieu chan) */
    GPIO_ResetBits(port, (uint16_t)(odr & pin));
    GPIO_SetBits(port, (uint16_t)(~odr & pin));
}

uint8_t hal_gpio_read(GPIO_TypeDef *port, uint16_t pin)
{
    return (GPIO_ReadInputDataBit(port, pin) == (uint8_t)Bit_SET) ? 1U : 0U;
}

/* ========================================================================== */
/*                                   EXTI                                     */
/* ========================================================================== */

/* Ham goi lai cho 16 line EXTI (chi so = so chan) */
static hal_exti_cb_t s_exti_cb[16];

/** @brief Doi mask chan sang so thu tu. Vd GPIO_Pin_2 (0x0004) -> 2. @return 16 neu pin sai. */
static uint8_t prvPinIndex(uint16_t pin)
{
    uint8_t i;
    for (i = 0U; i < 16U; i++) {
        if (pin == (uint16_t)(1U << i)) {
            return i;
        }
    }
    return 16U;
}

/** @brief Doi port sang ma "port source" cua SPL. @return 0xFF neu port sai. */
static uint8_t prvPortSource(GPIO_TypeDef *port)
{
    if (port == GPIOA) return GPIO_PortSourceGPIOA;
    if (port == GPIOB) return GPIO_PortSourceGPIOB;
    if (port == GPIOC) return GPIO_PortSourceGPIOC;
    if (port == GPIOD) return GPIO_PortSourceGPIOD;
    return 0xFFU;
}

/** @brief Vector ngat NVIC ung voi line EXTI. */
static uint8_t prvExtiIrq(uint8_t idx)
{
    switch (idx) {
    case 0:  return EXTI0_IRQn;
    case 1:  return EXTI1_IRQn;
    case 2:  return EXTI2_IRQn;
    case 3:  return EXTI3_IRQn;
    case 4:  return EXTI4_IRQn;
    default: return (idx <= 9U) ? (uint8_t)EXTI9_5_IRQn : (uint8_t)EXTI15_10_IRQn;
    }
}

uint8_t hal_exti_config(GPIO_TypeDef *port, uint16_t pin, uint8_t trigger,
                        uint8_t irq_prio, hal_exti_cb_t cb)
{
    EXTI_InitTypeDef ei;
    NVIC_InitTypeDef ni;
    uint8_t idx = prvPinIndex(pin);
    uint8_t src = prvPortSource(port);

    if (idx >= 16U || src == 0xFFU || trigger > HAL_EXTI_BOTH || irq_prio > 15U || cb == 0) {
        return 1U;
    }

    /* B1: noi chan (vd PA2) vao line EXTI cung so (line 2) */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_EXTILineConfig(src, idx);
    s_exti_cb[idx] = cb;

    /* B2: line EXTI che do ngat + canh kich */
    ei.EXTI_Line    = pin;                  /* EXTI_LineN == GPIO_Pin_N */
    ei.EXTI_Mode    = EXTI_Mode_Interrupt;
    ei.EXTI_Trigger = (trigger == HAL_EXTI_RISING)  ? EXTI_Trigger_Rising  :
                      (trigger == HAL_EXTI_FALLING) ? EXTI_Trigger_Falling :
                                                      EXTI_Trigger_Rising_Falling;
    ei.EXTI_LineCmd = ENABLE;
    EXTI_ClearITPendingBit(pin);
    EXTI_Init(&ei);

    /* B3: bat vector ngat trong NVIC */
    ni.NVIC_IRQChannel                   = prvExtiIrq(idx);
    ni.NVIC_IRQChannelPreemptionPriority = irq_prio;
    ni.NVIC_IRQChannelSubPriority        = 0;
    ni.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&ni);
    return 0U;
}

/**
 * @brief Xu ly ngat cho cac line first..last: line nao co co thi xoa co
 *        roi goi ham da dang ky. (Line 5..9 va 10..15 dung chung 1 vector.)
 */
static void prvExtiDispatch(uint8_t first, uint8_t last)
{
    uint8_t i;

    for (i = first; i <= last; i++) {
        uint32_t line = (1UL << i);
        if (EXTI_GetITStatus(line) == SET) {
            EXTI_ClearITPendingBit(line);
            if (s_exti_cb[i] != 0) {
                s_exti_cb[i]((uint16_t)line);
            }
        }
    }
}

/* Ten ham trung voi bang vector trong startup_stm32f10x_md.s */
void EXTI0_IRQHandler(void)     { prvExtiDispatch(0U, 0U);   }
void EXTI1_IRQHandler(void)     { prvExtiDispatch(1U, 1U);   }
void EXTI2_IRQHandler(void)     { prvExtiDispatch(2U, 2U);   }
void EXTI3_IRQHandler(void)     { prvExtiDispatch(3U, 3U);   }
void EXTI4_IRQHandler(void)     { prvExtiDispatch(4U, 4U);   }
void EXTI9_5_IRQHandler(void)   { prvExtiDispatch(5U, 9U);   }
void EXTI15_10_IRQHandler(void) { prvExtiDispatch(10U, 15U); }
