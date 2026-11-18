/*
 * =============================================================================
 *  File      : test/support/stm32f10x.h   (FILE GIA - CHI DUNG CHO UNIT TEST)
 *  Chuc nang : Thay the stm32f10x.h that (chi bien dich duoc cho chip ARM) de
 *              cac header hal_*.h / bsp.h bien dich duoc tren may tinh.
 *
 *  Chi khai bao dung nhung gi header can:
 *    - kieu GPIO_TypeDef, TIM_TypeDef, I2C_TypeDef
 *    - hang GPIOA.., TIM3, I2C1, GPIO_Pin_x
 *    - ham khoa ngat __get_PRIMASK / __disable_irq / __set_PRIMASK (khong lam gi)
 *  Cac con tro GPIOA... chi de SO SANH, khong bao gio duoc truy cap that
 *  (moi ham hal deu da bi thay bang mock).
 * =============================================================================
 */
#ifndef FAKE_STM32F10X_H
#define FAKE_STM32F10X_H

#include <stdint.h>

typedef struct { volatile uint32_t CRL, CRH, IDR, ODR, BSRR, BRR, LCKR; } GPIO_TypeDef;
typedef struct { volatile uint32_t dummy; } TIM_TypeDef;
typedef struct { volatile uint32_t dummy; } I2C_TypeDef;

/* Dia chi giong chip that, chi dung lam "ID" de so sanh */
#define GPIOA   ((GPIO_TypeDef *)0x40010800UL)
#define GPIOB   ((GPIO_TypeDef *)0x40010C00UL)
#define GPIOC   ((GPIO_TypeDef *)0x40011000UL)
#define GPIOD   ((GPIO_TypeDef *)0x40011400UL)
#define TIM2    ((TIM_TypeDef *)0x40000000UL)
#define TIM3    ((TIM_TypeDef *)0x40000400UL)
#define TIM4    ((TIM_TypeDef *)0x40000800UL)
#define I2C1    ((I2C_TypeDef *)0x40005400UL)
#define I2C2    ((I2C_TypeDef *)0x40005800UL)

#define GPIO_Pin_0   ((uint16_t)0x0001)
#define GPIO_Pin_1   ((uint16_t)0x0002)
#define GPIO_Pin_2   ((uint16_t)0x0004)
#define GPIO_Pin_3   ((uint16_t)0x0008)
#define GPIO_Pin_4   ((uint16_t)0x0010)
#define GPIO_Pin_5   ((uint16_t)0x0020)
#define GPIO_Pin_6   ((uint16_t)0x0040)
#define GPIO_Pin_7   ((uint16_t)0x0080)
#define GPIO_Pin_8   ((uint16_t)0x0100)
#define GPIO_Pin_9   ((uint16_t)0x0200)
#define GPIO_Pin_10  ((uint16_t)0x0400)
#define GPIO_Pin_11  ((uint16_t)0x0800)
#define GPIO_Pin_12  ((uint16_t)0x1000)
#define GPIO_Pin_13  ((uint16_t)0x2000)
#define GPIO_Pin_14  ((uint16_t)0x4000)
#define GPIO_Pin_15  ((uint16_t)0x8000)

/* Ham he thong goi trong bsp_system_init() (CMSIS + misc.c) -> khong lam gi */
#define NVIC_PriorityGroup_4   ((uint32_t)0x300)
static inline void SystemCoreClockUpdate(void)            { }
static inline void NVIC_PriorityGroupConfig(uint32_t grp) { (void)grp; }

/* Khoa/mo ngat: tren may tinh khong co ngat -> khong lam gi */
static inline uint32_t __get_PRIMASK(void)        { return 0U; }
static inline void     __disable_irq(void)        { }
static inline void     __set_PRIMASK(uint32_t pm) { (void)pm; }

#endif /* FAKE_STM32F10X_H */
