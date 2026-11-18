/*
 * =============================================================================
 *  File      : test_bsp.c
 *  Kieu test : Unity + CMock, kieu "interaction test"
 *  Doi tuong : bsp/bsp.c
 *
 *  Kiem tra tang bsp goi DUNG ham hal, DUNG chan, DUNG muc logic theo
 *  bang chan trong bsp.h. Moi lan goi ham hal phai duoc "Expect" truoc,
 *  goi sai/thua/thieu -> test FAIL.
 * =============================================================================
 */
#include "unity.h"
#include "bsp.h"
#include "mock_hal_gpio.h"
#include "mock_hal_delay.h"
#include "mock_hal_pwm.h"
#include "mock_hal_i2c.h"

/* ------------------- Bat callback ngat ma bsp dang ky -------------------- */

static hal_exti_cb_t s_isr[16];             /* callback bsp gui cho hal, theo chan */
static int           s_btn_hits[BSP_BTN_COUNT];

static void capture_exti(GPIO_TypeDef *port, uint16_t pin, hal_exti_trigger_t trig,
                         uint8_t prio, hal_exti_cb_t cb, int n)
{
    int i;
    (void)port; (void)trig; (void)prio; (void)n;
    for (i = 0; i < 16; i++) {
        if (pin == (uint16_t)(1U << i)) s_isr[i] = cb;
    }
}

static void app_button_cb(bsp_button_t btn)
{
    s_btn_hits[btn]++;
}

void setUp(void)
{
    int i;
    for (i = 0; i < BSP_BTN_COUNT; i++) s_btn_hits[i] = 0;
    for (i = 0; i < 16; i++) s_isr[i] = 0;
}

void tearDown(void)
{
}

/* ================================ LED ==================================== */

void test_led_on_drives_pin_low_because_active_low(void)
{
    hal_gpio_write_Expect(GPIOC, GPIO_Pin_13, 0);
    bsp_led_set(1);
}

void test_led_off_drives_pin_high(void)
{
    hal_gpio_write_Expect(GPIOC, GPIO_Pin_13, 1);
    bsp_led_set(0);
}

/* ================================ Coi ==================================== */

void test_buzzer_on_and_off(void)
{
    hal_gpio_write_Expect(GPIOB, GPIO_Pin_12, 1);
    bsp_buzzer_set(1);
    hal_gpio_write_Expect(GPIOB, GPIO_Pin_12, 0);
    bsp_buzzer_set(0);
}

/* ================================ Quat =================================== */

void test_fan_set_updates_pwm_then_enables_power(void)
{
    hal_pwm_set_duty_Expect(TIM3, 1, 60);
    hal_gpio_write_Expect(GPIOB, GPIO_Pin_1, 1);
    bsp_fan_set(60);
}

void test_fan_zero_disables_power_stage(void)
{
    hal_pwm_set_duty_Expect(TIM3, 1, 0);
    hal_gpio_write_Expect(GPIOB, GPIO_Pin_1, 0);
    bsp_fan_set(0);
}

/* ============================== Nut nhan ================================= */

void test_button_pressed_when_pin_low(void)
{
    hal_gpio_read_ExpectAndReturn(GPIOB, GPIO_Pin_14, 0);
    TEST_ASSERT_EQUAL_UINT8(1, bsp_button_pressed(BSP_BTN_UP));
}

void test_button_released_when_pin_high(void)
{
    hal_gpio_read_ExpectAndReturn(GPIOB, GPIO_Pin_15, 1);
    TEST_ASSERT_EQUAL_UINT8(0, bsp_button_pressed(BSP_BTN_DOWN));
}

void test_invalid_button_id_reads_nothing(void)
{
    /* Khong Expect gi: neu bsp goi hal -> CMock bao loi */
    TEST_ASSERT_EQUAL_UINT8(0, bsp_button_pressed(BSP_BTN_COUNT));
}

void test_button_irq_enable_clears_pending_then_unmasks(void)
{
    hal_exti_clear_Expect(GPIO_Pin_13);
    hal_exti_enable_Expect(GPIO_Pin_13);
    bsp_button_irq_enable(BSP_BTN_SET, 1);
}

void test_button_irq_disable_masks_line(void)
{
    hal_exti_disable_Expect(GPIO_Pin_14);
    bsp_button_irq_enable(BSP_BTN_UP, 0);
}

void test_buttons_irq_init_configures_3_falling_edge_lines_with_rtos_safe_priority(void)
{
    hal_exti_config_Expect(GPIOB, GPIO_Pin_13, HAL_EXTI_FALLING, BSP_EXTI_IRQ_PRIO, NULL);
    hal_exti_config_IgnoreArg_cb();
    hal_exti_config_Expect(GPIOB, GPIO_Pin_14, HAL_EXTI_FALLING, BSP_EXTI_IRQ_PRIO, NULL);
    hal_exti_config_IgnoreArg_cb();
    hal_exti_config_Expect(GPIOB, GPIO_Pin_15, HAL_EXTI_FALLING, BSP_EXTI_IRQ_PRIO, NULL);
    hal_exti_config_IgnoreArg_cb();

    bsp_buttons_irq_init(app_button_cb);

    /* FreeRTOS: ngat goi API ...FromISR phai co uu tien so >= 5 */
    TEST_ASSERT_GREATER_OR_EQUAL(5, BSP_EXTI_IRQ_PRIO);
}

void test_exti_interrupt_is_translated_to_button_id(void)
{
    hal_exti_config_Stub(capture_exti);
    bsp_buttons_irq_init(app_button_cb);

    /* Gia lap phan cung bao ngat tren chan PB14 (nut UP) */
    TEST_ASSERT_NOT_NULL(s_isr[14]);
    s_isr[14](GPIO_Pin_14);

    TEST_ASSERT_EQUAL_INT(0, s_btn_hits[BSP_BTN_SET]);
    TEST_ASSERT_EQUAL_INT(1, s_btn_hits[BSP_BTN_UP]);
    TEST_ASSERT_EQUAL_INT(0, s_btn_hits[BSP_BTN_DOWN]);
}
