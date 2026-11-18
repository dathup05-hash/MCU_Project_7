/*
 * =============================================================================
 *  File      : task_display.c
 *  Tang      : app (FreeRTOS task)
 *  Task      : DisplayTask ("DISPLAY"), uu tien 1 (thap nhat), chu ky 250 ms
 *  Thiet bi  : OLED (bsp_oled) - CHI task nay ve man hinh.
 *  Chuc nang : Ve thong tin len OLED 128x64 (8 dong x 21 ky tu).
 *
 *  Bo cuc man hinh:
 *    Dong 0 : NHIET DO: 28.5 oC  (o = ky tu dau do)
 *    Dong 1 : ---------------- (gach ngang)
 *    Dong 3 : Nguong quat: 35.0
 *    Dong 4 : Canh bao   : 40.0   (co dinh)
 *    Dong 5 : Quat: BAT
 *    Dong 7 : QUA NHIET / goi y cach bam nut
 * =============================================================================
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "task_display.h"
#include "app_ipc.h"
#include "app_cfg.h"
#include "app_settings.h"
#include "app_logic.h"
#include "bsp_oled.h"

static TaskHandle_t xDisplayTaskHandle = NULL;


/**
 * @brief  Thu thap du lieu tu cac task khac va ve toan bo man hinh vao bo dem RAM.
 *         (Chua day len OLED - viec do do bsp_oled_flush() lam.)
 */
static void prvRender(void)
{
    TempData_t    temp;
    FanStatus_t   fan = { 0U };
    AppSettings_t s;
    EventBits_t   ev = xEventGroupGetBits(xUiEvents);   /* doc co, khong xoa */
    char          v[12];        /* chuoi gia tri */
    char          line[22];     /* 1 dong man hinh */

    app_settings_get(&s);
    (void)xQueuePeek(xFanQueue, &fan, 0);

    bsp_oled_clear();

    /* Dong 0: nhiet do ('\x7F' la ky tu dau do trong font) */
    if (ev & EVT_SENSOR_ERROR) {
        bsp_oled_puts(0, 0, "NHIET DO: LOI CB");
    } else if (xQueuePeek(xTempQueue, &temp, 0) == pdPASS && temp.valid) {
        app_logic_fmt_x10(v, sizeof(v), temp.temp_x10);
        snprintf(line, sizeof(line), "NHIET DO: %s\x7F" "C", v);
        bsp_oled_puts(0, 0, line);
    } else {
        bsp_oled_puts(0, 0, "NHIET DO: --.-");      /* chua doc lan nao */
    }
    bsp_oled_hline(1, 3);       /* gach ngang o giua dong 1 */

    /* Dong 3: nguong quat */
    app_logic_fmt_x10(v, sizeof(v), s.t_set_x10);
    snprintf(line, sizeof(line), "Nguong quat: %s", v);
    bsp_oled_puts(0, 3, line);

    /* Dong 4: nguong canh bao co dinh */
    app_logic_fmt_x10(v, sizeof(v), ALARM_TEMP_X10);
    snprintf(line, sizeof(line), "Canh bao   : %s", v);
    bsp_oled_puts(0, 4, line);

    /* Dong 5: quat */
    bsp_oled_puts(0, 5, fan.on ? "Quat: BAT" : "Quat: TAT");

    /* Dong 7: canh bao (uu tien) hoac goi y thao tac */
    if ((ev & EVT_ALARM_ACTIVE) != 0U) {
        bsp_oled_puts(0, 7, ((ev & EVT_ALARM_MUTED) != 0U) ? "QUA NHIET (TAT TIENG)"
                                                         : "!!! QUA NHIET !!!");
    } else {
        bsp_oled_puts(0, 7, "UP/DN:chinh RST:goc");
    }
}

/**
 * @brief  Than task: khoi tao OLED (thu lai neu loi), roi ve lai lien tuc.
 */
static void prvDisplayTask(void *pvParameters)
{
    uint8_t ready = 0;      /* 1 = OLED da khoi tao xong */

    (void)pvParameters;

    for (;;) {
        /* Khoi tao OLED trong task (sau khi scheduler chay) de dung duoc Mutex I2C */
        if (!ready) {
            ready = (uint8_t)(bsp_oled_init() == 0U);
            if (!ready) {
                vTaskDelay(pdMS_TO_TICKS(500));    /* chua cam OLED -> 0.5s thu lai */
                continue;
            }
        }

        prvRender();
        if (bsp_oled_flush() != 0U) {
            ready = 0;                              /* mat ket noi -> khoi tao lai */
        }

        /* Ngu toi da 250ms; neu InputTask bao EVT_UI_CHANGED thi day ngay
         * -> bam nut la man hinh cap nhat lien. */
        xEventGroupWaitBits(xUiEvents, EVT_UI_CHANGED, pdTRUE, pdFALSE,
                            pdMS_TO_TICKS(TASK_DISPLAY_PERIOD_MS));
    }
}

BaseType_t task_display_create(void)
{
    return xTaskCreate(prvDisplayTask, "DISPLAY", TASK_DISPLAY_STACK,
                       NULL, TASK_DISPLAY_PRIO, &xDisplayTaskHandle);
}
