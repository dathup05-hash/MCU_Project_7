#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H

/*
 * Chon cac module SPL duoc dung trong du an.
 * Them module moi: chep .h/.c tu SPL V3.5.0 vao middlewares/STM32F10x_StdPeriph_Driver
 * roi bo comment dong #include tuong ung.
 */
/* #include "stm32f10x_adc.h" */
#include "stm32f10x_exti.h"
#include "stm32f10x_flash.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_usart.h"
/* #include "stm32f10x_usart.h" */
#include "misc.h"   /* NVIC */

#ifdef USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t *file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif

#endif /* __STM32F10x_CONF_H */
