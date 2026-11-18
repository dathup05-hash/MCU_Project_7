/*
 * =============================================================================
 *  File      : task_sensor.c
 *  Tang      : app (FreeRTOS task)
 *  Task      : TemperatureTask ("SENSOR"), uu tien 3, chu ky 1 giay
 *  Thiet bi  : DS18B20 (bsp_ds18b20) - CHI task nay doc cam bien.
 *  Chuc nang : Doc nhiet do roi gui vao mailbox xTempQueue
 *              de ControlTask, AlarmTask, DisplayTask cung doc.
 *              In nhiet do ra UART debug moi giay.
 *
 *  Xu ly loi:
 *    - Loi le te (< 3 lan lien tiep): giu gia tri cu, khong bao loi
 *    - Loi >= 3 lan lien tiep: gui valid = 0 va bat co EVT_SENSOR_ERROR
 * =============================================================================
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "task_sensor.h"
#include "app_ipc.h"
#include "app_cfg.h"
#include "app_logic.h"
#include "bsp_ds18b20.h"
#include "bsp_uart.h"

static TaskHandle_t xSensorTaskHandle = NULL;

/** @brief In nhiet do ra UART, vd "T = 28.5 C" hoac "T = LOI CAM BIEN". */
static void prvLog(const TempData_t *d)
{
    char v[8];
    char line[24];

    if (!d->valid) {
        bsp_uart_println("T = LOI CAM BIEN");
        return;
    }
    app_logic_fmt_x10(v, sizeof(v), d->temp_x10);
    snprintf(line, sizeof(line), "T = %s C", v);
    bsp_uart_println(line);
}

/**
 * @brief  Than task: vong lap vo han, moi vong ~1 giay.
 */
static void prvSensorTask(void *pvParameters)
{
    TempData_t data = { 0, 0U, 0U };    /* du lieu gui di                    */
    uint8_t    fail = 0U;               /* dem so lan doc loi lien tiep      */
    int16_t    t;                       /* nhiet do vua doc (x10 do C)       */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_SENSOR_PERIOD_MS);

    (void)pvParameters;

    for (;;) {
        /* B1: ra lenh do, cho 750 ms (task ngu, nhuong CPU cho task khac) */
        if (bsp_ds18b20_start_conversion() == 0U) {
            vTaskDelay(pdMS_TO_TICKS(DS18B20_CONV_TIME_MS));
        }

        /* B2: doc ket qua */
        if (bsp_ds18b20_read_temp(&t) == 0U) {
            data.temp_x10 = t;
            data.valid    = 1U;
            fail          = 0U;
        } else if (fail < SENSOR_MAX_FAIL) {
            fail++;
        }

        /* B3: cap nhat co loi cam bien */
        if (fail >= SENSOR_MAX_FAIL) {
            data.valid = 0U;
            xEventGroupSetBits(xUiEvents, EVT_SENSOR_ERROR);
        } else {
            xEventGroupClearBits(xUiEvents, EVT_SENSOR_ERROR);
        }

        /* B4: gui vao mailbox (ghi de gia tri cu). Loi le te thi bo qua lan nay. */
        if (fail == 0U || fail >= SENSOR_MAX_FAIL) {
            data.tick = xTaskGetTickCount();
            xQueueOverwrite(xTempQueue, &data);
            prvLog(&data);
        }

        /* Ngu den dung moc 1 giay ke tiep (khong bi troi chu ky) */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

BaseType_t task_sensor_create(void)
{
    return xTaskCreate(prvSensorTask,       /* ham than task          */
                       "SENSOR",            /* ten (de debug)          */
                       TASK_SENSOR_STACK,   /* stack (word)            */
                       NULL,                /* tham so truyen vao task */
                       TASK_SENSOR_PRIO,    /* do uu tien              */
                       &xSensorTaskHandle); /* handle tra ve           */
}
