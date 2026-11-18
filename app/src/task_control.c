/*
 * =============================================================================
 *  File      : task_control.c
 *  Tang      : app (FreeRTOS task)
 *  Task      : ControlTask ("CONTROL"), uu tien 3, chu ky 500 ms
 *  Thiet bi  : Relay quat (bsp_relay) - CHI task nay dieu khien relay.
 *  Chuc nang : Quyet dinh BAT / TAT quat theo nhiet do va nguong.
 *
 *  Thu tu uu tien (xem app_logic_fan_on):
 *    1. Mat cam bien             -> BAT (an toan)
 *    2. T >= nguong              -> BAT
 *    3. T <  nguong - 1.0 C      -> TAT
 *    4. Nam giua                 -> giu nguyen (do tre, relay khong dong ngat lien tuc)
 *
 *    Quat
 *     BAT |            +-----------------     nong len toi nguong -> BAT
 *         |            |   ^
 *         |            v   |    vung tre 1.0 C: giu nguyen
 *     TAT |------------+                      nguoi xuong duoi (nguong - 1.0) -> TAT
 *         +--------+---+------------> nhiet do
 *           nguong-1.0  nguong
 * =============================================================================
 */
#include "FreeRTOS.h"
#include "task.h"
#include "task_control.h"
#include "app_ipc.h"
#include "app_cfg.h"
#include "app_settings.h"
#include "app_logic.h"
#include "bsp_relay.h"
#include "bsp_uart.h"

static TaskHandle_t xControlTaskHandle = NULL;

/**
 * @brief  Than task: moi 500 ms doc nhiet do + nguong, quyet dinh, dieu khien relay.
 */
static void prvControlTask(void *pvParameters)
{
    TempData_t    temp = { 0, 0U, 0U };
    AppSettings_t s;
    FanStatus_t   fan  = { 0U };
    uint8_t       has_temp;
    uint8_t       on;
    TickType_t    xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_CONTROL_PERIOD_MS);

    (void)pvParameters;

    for (;;) {
        /* 1. DOC: nhiet do (do SensorTask gui) + nguong (do InputTask chinh) */
        app_settings_get(&s);
        has_temp = (uint8_t)(xQueuePeek(xTempQueue, &temp, 0) == pdPASS);

        /* 2. TINH */
        on = app_logic_fan_on(has_temp, temp.valid, temp.temp_x10, &s, fan.on);

        /* 3. XUAT: relay (chi doi khi can) + bao trang thai cho DisplayTask */
        if (on != fan.on) {
            bsp_relay_set(on);
            bsp_uart_println(on ? "QUAT: BAT" : "QUAT: TAT");
            if (on) {
                xEventGroupSetBits(xUiEvents, EVT_FAN_ON);  /* AlarmTask bip bip */
            }
        }
        fan.on = on;
        xQueueOverwrite(xFanQueue, &fan);

        /* 4. NGU */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

BaseType_t task_control_create(void)
{
    return xTaskCreate(prvControlTask, "CONTROL", TASK_CONTROL_STACK,
                       NULL, TASK_CONTROL_PRIO, &xControlTaskHandle);
}
