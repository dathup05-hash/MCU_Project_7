/*
 * =============================================================================
 *  File      : task_input.c
 *  Tang      : app (FreeRTOS task)
 *  Task      : InputTask ("INPUT"), uu tien 4 (cao nhat), chay khi co nut bam
 *  Thiet bi  : 3 nut nhan (bsp_button) - CHI task nay xu ly nut.
 *  Chuc nang : Chinh NGUONG QUAT bang nut, in gia tri moi ra UART.
 *
 *    UP    : +0.5 C moi lan bam
 *    DOWN  : -0.5 C moi lan bam
 *    RESET : bam roi NHA (< 2 s) -> nguong ve mac dinh (35.0 C)
 *            GIU 2 s             -> tat tieng coi bao dong (khong doi nguong)
 *
 *  Luong xu ly:
 *    Bam nut -> canh xuong -> NGAT -> prvButtonIsr() gui ID nut vao xButtonQueue
 *            -> task dang ngu trong xQueueReceive() thuc day -> xu ly
 *
 *  Chong doi phim (nut co hoc "nay" vai ms khi bam/nha), 2 lop:
 *    1. Trong ngat : bo qua canh xuong cach lan duoc nhan truoc < 50 ms
 *    2. Trong task : cho 20 ms roi doc lai chan, nut khong con bam -> nhieu, bo qua
 *
 *  Task KHONG dieu khien coi / man hinh. Chi BAO TIN:
 *    EVT_UI_CHANGED -> DisplayTask ve lai ngay
 *    EVT_BEEP       -> AlarmTask bip ngan
 *    EVT_MUTE_REQ   -> AlarmTask tat tieng bao dong
 * =============================================================================
 */
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "task_input.h"
#include "app_ipc.h"
#include "app_cfg.h"
#include "app_settings.h"
#include "app_logic.h"
#include "bsp_button.h"
#include "bsp_uart.h"

static TaskHandle_t xInputTaskHandle = NULL;

/* Thoi diem (tick) lan cuoi nhan canh xuong cua tung nut - dung trong ngat */
static TickType_t s_last_tick[BSP_BTN_COUNT];

/**
 * @brief  Chay TRONG NGAT moi khi 1 nut co canh xuong.
 *         Loc doi lop 1 roi gui ID nut cho task. Chi dung API ...FromISR().
 */
static void prvButtonIsr(uint8_t btn)
{
    BaseType_t woken = pdFALSE;
    TickType_t now   = xTaskGetTickCountFromISR();

    if ((now - s_last_tick[btn]) < pdMS_TO_TICKS(BTN_DEBOUNCE_MS)) {
        return;                                     /* van dang "nay" -> bo qua */
    }
    s_last_tick[btn] = now;

    (void)xQueueSendFromISR(xButtonQueue, &btn, &woken);
    portYIELD_FROM_ISR(woken);                      /* chuyen ngay sang InputTask neu can */
}

/** @brief In nguong hien tai ra UART, vd "[UP] Nguong quat = 35.5 C". */
static void prvPrint(const char *tag, const AppSettings_t *s)
{
    char v_set[8];
    char line[40];

    app_logic_fmt_x10(v_set, sizeof(v_set), s->t_set_x10);
    snprintf(line, sizeof(line), "[%s] Nguong quat = %s C", tag, v_set);
    bsp_uart_println(line);
}

/** @brief Doi nguong (UP / DOWN / RESET), bao man hinh ve lai + bip. */
static void prvChangeThreshold(uint8_t id)
{
    static const char *const tags[BSP_BTN_COUNT] = { "UP", "DOWN", "RESET" };
    AppSettings_t s;

    app_settings_get(&s);
    if (id == BSP_BTN_RESET) {
        app_logic_settings_defaults(&s);
    } else {
        app_logic_settings_adjust(&s, (id == BSP_BTN_UP) ? (int8_t)1 : (int8_t)-1);
    }
    app_settings_set(&s);
    prvPrint(tags[id], &s);
    xEventGroupSetBits(xUiEvents, EVT_UI_CHANGED | EVT_BEEP);
}

/**
 * @brief  Nut RESET: do xem nguoi dung giu bao lau.
 *         Nha truoc 2 s -> reset nguong. Giu du 2 s -> tat tieng coi.
 */
static void prvHandleReset(void)
{
    uint32_t held_ms = BTN_CONFIRM_MS;

    while (bsp_button_pressed(BSP_BTN_RESET) && held_ms < BTN_MUTE_HOLD_MS) {
        vTaskDelay(pdMS_TO_TICKS(BTN_CONFIRM_MS));
        held_ms += BTN_CONFIRM_MS;
    }

    if (held_ms >= BTN_MUTE_HOLD_MS) {
        xEventGroupSetBits(xUiEvents, EVT_MUTE_REQ | EVT_UI_CHANGED);
        bsp_uart_println("[RESET giu 2 s] Tat tieng coi bao dong");
    } else {
        prvChangeThreshold(BSP_BTN_RESET);
    }
}

/**
 * @brief  Than task: ngu cho den khi co nut bam (khong ton CPU).
 */
static void prvInputTask(void *pvParameters)
{
    AppSettings_t s;
    uint8_t       id;

    (void)pvParameters;

    bsp_uart_println("NUT: A2 = tang, A3 = giam, A4 = reset nguong (giu 2 s = tat coi)");
    app_settings_get(&s);
    prvPrint("START", &s);

    bsp_button_irq_init(prvButtonIsr);      /* bat ngat 3 nut (hang doi da tao trong main) */

    for (;;) {
        if (xQueueReceive(xButtonQueue, &id, portMAX_DELAY) != pdPASS) {
            continue;
        }

        /* Loc doi lop 2: cho het nay roi kiem tra nut con bam that khong */
        vTaskDelay(pdMS_TO_TICKS(BTN_CONFIRM_MS));
        if (!bsp_button_pressed(id)) {
            continue;                       /* xung nhieu, bo qua */
        }

        if (id == BSP_BTN_RESET) {
            prvHandleReset();
        } else {
            prvChangeThreshold(id);
        }
    }
}

BaseType_t task_input_create(void)
{
    return xTaskCreate(prvInputTask, "INPUT", TASK_INPUT_STACK,
                       NULL, TASK_INPUT_PRIO, &xInputTaskHandle);
}
