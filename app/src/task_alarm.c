/*
 * =============================================================================
 *  File      : task_alarm.c
 *  Tang      : app (FreeRTOS task)
 *  Task      : AlarmTask ("ALARM"), uu tien 2, nhip 10 ms
 *  Thiet bi  : Coi (PB12) + LED PC13 - CHI task nay dieu khien coi va LED.
 *              Task khac muon coi keu thi dat co su kien, KHONG goi bsp_buzzer_set.
 *
 *    Tinh huong                     Coi                         LED PC13
 *    -----------------------------  --------------------------  -----------
 *    Qua nhiet (T >= 40.0 C)        bip-bip-bip, lap moi ~1 s   nhay nhanh
 *    Mat cam bien                   bip dai 1 tieng moi 5 s     nhay cham
 *    Quat vua bat (EVT_FAN_ON)      bip bip (2 tieng ngan)      -
 *    Bam nut (EVT_BEEP)             bip ngan 30 ms              -
 *    Binh thuong                    im                          tat
 *
 *  Tat tieng: giu nut RESET 2 s (EVT_MUTE_REQ) -> coi bao dong im, LED van nhay.
 *  Het qua nhiet / co lai cam bien -> tu huy tat tieng.
 *
 *  Cach keu: moi "mau keu" la 1 bang thoi gian (ms) BAT, TAT, BAT, TAT ... ket
 *  thuc bang 0. Task chay moi 10 ms, dem lui va doi buoc -> khong bi chan.
 *  Uu tien: bao dong (qua nhiet > mat cam bien) > bip 1 lan (quat bat / bam nut).
 * =============================================================================
 */
#include "FreeRTOS.h"
#include "task.h"
#include "task_alarm.h"
#include "app_ipc.h"
#include "app_cfg.h"
#include "app_logic.h"
#include "bsp.h"

/* --------------------------- Bang mau keu (ms) ----------------------------- */
/* Phan tu chan = coi BAT, phan tu le = coi TAT, 0 = het bang */
static const uint16_t s_pat_click[]    = { 30, 0 };
static const uint16_t s_pat_fan_on[]   = { 60, 80, 60, 0 };
static const uint16_t s_pat_overheat[] = { 80, 80, 80, 80, 80, 600, 0 };   /* lap lai */
static const uint16_t s_pat_sensor[]   = { 400, 4600, 0 };                 /* lap lai */

#define LED_FAST_HALF_MS    125     /* qua nhiet: LED doi trang thai moi 125 ms (4 Hz) */
#define LED_SLOW_HALF_MS    500     /* mat cam bien: moi 500 ms (1 Hz)                 */

/** Trinh phat 1 mau keu */
typedef struct {
    const uint16_t *pat;    /* bang dang phat, NULL = khong phat */
    uint8_t         idx;    /* buoc hien tai trong bang          */
    uint16_t        left;   /* so ms con lai cua buoc hien tai   */
    uint8_t         loop;   /* 1 = het bang thi lap lai          */
} Player_t;

static TaskHandle_t xAlarmTaskHandle = NULL;

/** @brief Bat dau phat 1 mau keu tu dau. */
static void prvPlay(Player_t *p, const uint16_t *pat, uint8_t loop)
{
    p->pat  = pat;
    p->idx  = 0U;
    p->left = (pat != NULL) ? pat[0] : 0U;
    p->loop = loop;
}

/**
 * @brief  Chay trinh phat them 1 nhip (TASK_ALARM_TICK_MS).
 * @return 1 = coi phai BAT o nhip nay, 0 = TAT
 */
static uint8_t prvStep(Player_t *p)
{
    uint8_t on;

    if (p->pat == NULL) {
        return 0U;
    }
    on = (uint8_t)((p->idx & 1U) == 0U);        /* buoc chan = BAT */

    if (p->left > TASK_ALARM_TICK_MS) {
        p->left = (uint16_t)(p->left - TASK_ALARM_TICK_MS);
    } else {
        p->idx++;
        if (p->pat[p->idx] == 0U) {             /* het bang */
            if (p->loop) {
                p->idx = 0U;
            } else {
                p->pat = NULL;
                return on;
            }
        }
        p->left = p->pat[p->idx];
    }
    return on;
}

/**
 * @brief Than task: moi 10 ms doc trang thai, chon mau keu, xuat coi + LED.
 */
static void prvAlarmTask(void *pvParameters)
{
    Player_t   bg  = { NULL, 0U, 0U, 0U };  /* mau bao dong (lap lai)          */
    Player_t   one = { NULL, 0U, 0U, 0U };  /* bip 1 lan (bam nut / quat bat)  */
    const uint16_t *bg_want;                /* mau bao dong can phat luc nay   */
    TempData_t temp;
    EventBits_t ev, req;
    uint8_t    over = 0U;                   /* 1 = dang qua nhiet              */
    uint8_t    sensor_err;
    uint8_t    muted = 0U;                  /* 1 = da tat tieng bao dong       */
    uint8_t    buzzer, led;
    TickType_t now;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_ALARM_TICK_MS);

    (void)pvParameters;

    for (;;) {
        /* 1. DOC: lay va xoa cac co yeu cau (ham tra ve gia tri TRUOC khi xoa) */
        req = xEventGroupClearBits(xUiEvents, EVT_BEEP | EVT_FAN_ON | EVT_MUTE_REQ);
        ev  = xEventGroupGetBits(xUiEvents);
        sensor_err = (uint8_t)((ev & EVT_SENSOR_ERROR) != 0U);

        /* 2. TINH: qua nhiet? (nguong co dinh 40.0 C, co do tre) */
        if (!sensor_err && xQueuePeek(xTempQueue, &temp, 0) == pdPASS && temp.valid) {
            over = app_logic_alarm_over(over, temp.temp_x10);
        } else {
            over = 0U;
        }

        /* Tat tieng: chi co tac dung khi dang bao dong; het bao dong thi tu huy */
        if (!over && !sensor_err) {
            muted = 0U;
        } else if ((req & EVT_MUTE_REQ) != 0U) {
            muted = 1U;
        }

        /* Bao DisplayTask */
        if (over) {
            xEventGroupSetBits(xUiEvents, EVT_ALARM_ACTIVE);
        } else {
            xEventGroupClearBits(xUiEvents, EVT_ALARM_ACTIVE);
        }
        if (muted) {
            xEventGroupSetBits(xUiEvents, EVT_ALARM_MUTED);
        } else {
            xEventGroupClearBits(xUiEvents, EVT_ALARM_MUTED);
        }

        /* Chon mau bao dong; doi mau thi phat lai tu dau */
        if (muted) {
            bg_want = NULL;
        } else if (over) {
            bg_want = s_pat_overheat;
        } else if (sensor_err) {
            bg_want = s_pat_sensor;
        } else {
            bg_want = NULL;
        }
        if (bg_want != bg.pat) {
            prvPlay(&bg, bg_want, 1U);
        }

        /* Bip 1 lan: quat bat uu tien hon bam nut */
        if ((req & EVT_FAN_ON) != 0U) {
            prvPlay(&one, s_pat_fan_on, 0U);
        } else if ((req & (EVT_BEEP | EVT_MUTE_REQ)) != 0U) {
            prvPlay(&one, s_pat_click, 0U);
        }

        /* 3. XUAT: bao dong dang keu thi che bip 1 lan */
        buzzer = prvStep(&one);
        if (bg.pat != NULL) {
            buzzer = prvStep(&bg);
        }
        bsp_buzzer_set(buzzer);

        now = xTaskGetTickCount();
        if (over) {
            led = (uint8_t)((now / pdMS_TO_TICKS(LED_FAST_HALF_MS)) & 1U);
        } else if (sensor_err) {
            led = (uint8_t)((now / pdMS_TO_TICKS(LED_SLOW_HALF_MS)) & 1U);
        } else {
            led = 0U;
        }
        bsp_led_set(led);

        /* 4. NGU toi nhip ke tiep */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

BaseType_t task_alarm_create(void)
{
    return xTaskCreate(prvAlarmTask, "ALARM", TASK_ALARM_STACK,
                       NULL, TASK_ALARM_PRIO, &xAlarmTaskHandle);
}
