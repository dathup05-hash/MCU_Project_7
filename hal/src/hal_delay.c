/*
 * =============================================================================
 *  File      : hal_delay.c
 *  Tang      : hal
 *  Chuc nang : Delay chan (2 vong for) + delay/millis dung FreeRTOS.
 *              Xem mo ta trong hal_delay.h.
 *
 *  HIEU CHINH HAL_DELAY_LOOPS_PER_US tren kit (lam 1 lan):
 *    Trong 1 task:
 *        uint32_t t0 = hal_millis();
 *        hal_delay_us(1000000);              // 1 giay
 *        g_dt = hal_millis() - t0;           // xem g_dt trong Watch cua Keil
 *    Dung thi g_dt ~ 1000. Neu g_dt = 1250 thi giam hang so: 12 * 1000 / 1250 = 9.6 -> 10.
 *    Cong thuc: gia tri moi = gia tri cu * 1000 / g_dt.
 * =============================================================================
 */
#include "hal_delay.h"
#include "stm32f10x.h"      /* __NOP() */
#include "FreeRTOS.h"
#include "task.h"

/* So vong lap trong (vong j) cho 1 micro-giay o 72 MHz.
 * Uoc tinh: moi vong j ~6 chu ky CPU (NOP + tru + re nhanh + cho Flash)
 * -> 72 / 6 = 12. Phai hieu chinh lai tren kit theo huong dan o tren. */
#define HAL_DELAY_LOOPS_PER_US  12U

void hal_delay_us(uint32_t us)
{
    uint32_t i, j;

    for (i = 0U; i < us; i++) {                     /* vong ngoai: 1 lan = 1 us   */
        for (j = 0U; j < HAL_DELAY_LOOPS_PER_US; j++) {
            __NOP();                                /* khong cho compiler xoa vong lap */
        }
    }
}

void hal_delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        hal_delay_us(ms * 1000U);       /* trong main(): chua co task de ngu */
    } else {
        vTaskDelay(pdMS_TO_TICKS(ms));  /* trong task: ngu, nhuong CPU      */
    }
}

uint32_t hal_millis(void)
{
    return (uint32_t)xTaskGetTickCount() * (uint32_t)portTICK_PERIOD_MS;
}
