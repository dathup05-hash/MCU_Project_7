/*
 * =============================================================================
 *  File      : app_ipc.h
 *  Tang      : app
 *  Chuc nang : Khai bao cac doi tuong IPC (Inter-Process Communication) cua
 *              FreeRTOS de cac task trao doi du lieu an toan voi nhau.
 *
 *  So do luong du lieu (moi thiet bi chi do 1 task dieu khien):
 *
 *    SensorTask  --(xTempQueue)-->  ControlTask, AlarmTask, DisplayTask
 *    ControlTask --(xFanQueue)--->  DisplayTask
 *    Ngat nut    --(xButtonQueue)-----------------> InputTask
 *    InputTask   --(xUiEvents: UI_CHANGED)--------> DisplayTask
 *    SensorTask  --(xUiEvents: SENSOR_ERROR)------> AlarmTask, DisplayTask
 *    InputTask   --(xUiEvents: BEEP, MUTE_REQ)----> AlarmTask
 *    ControlTask --(xUiEvents: FAN_ON)------------> AlarmTask
 *    AlarmTask   --(xUiEvents: ALARM_ACTIVE, ALARM_MUTED)--> DisplayTask
 *    Nguong quat: InputTask ghi, task khac doc qua app_settings (co Mutex)
 *
 *  "Mailbox" = queue chi co 1 phan tu:
 *    - Ben gui dung xQueueOverwrite(): luon ghi de gia tri moi nhat
 *    - Ben nhan dung xQueuePeek(): doc ma KHONG lay ra -> nhieu task cung doc duoc
 * =============================================================================
 */
#ifndef APP_IPC_H
#define APP_IPC_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "event_groups.h"

/** Du lieu nhiet do SensorTask gui di (mailbox xTempQueue) */
typedef struct {
    int16_t    temp_x10;    /* nhiet do, don vi 0.1 do C (285 = 28.5 C)  */
    uint8_t    valid;       /* 1 = doc thanh cong, 0 = loi cam bien      */
    TickType_t tick;        /* thoi diem doc (so tick tu luc khoi dong)  */
} TempData_t;

/** Trang thai quat ControlTask gui di (mailbox xFanQueue) */
typedef struct {
    uint8_t on;             /* 1 = relay dong, quat chay; 0 = quat tat */
} FanStatus_t;

/* ---- Cac handle IPC (tao trong app_ipc_init) ---- */
extern QueueHandle_t      xTempQueue;    /* mailbox nhiet do                       */
extern QueueHandle_t      xFanQueue;     /* mailbox trang thai quat                */
extern QueueHandle_t      xButtonQueue;  /* hang doi ID nut bam (uint8_t), ngat -> InputTask */
extern EventGroupHandle_t xUiEvents;     /* nhom co bao hieu su kien giua cac task */

/* ---- Cac bit (co) trong xUiEvents ---- */
#define EVT_UI_CHANGED      ((EventBits_t)(1U << 0))   /* vua bam nut -> ve lai man hinh ngay */
#define EVT_BEEP            ((EventBits_t)(1U << 1))   /* yeu cau bip ngan (bam nut)          */
#define EVT_ALARM_ACTIVE    ((EventBits_t)(1U << 2))   /* dang QUA NHIET (>= 40.0 C)          */
#define EVT_SENSOR_ERROR    ((EventBits_t)(1U << 3))   /* MAT cam bien                        */
#define EVT_FAN_ON          ((EventBits_t)(1U << 4))   /* quat vua chuyen tu TAT sang BAT     */
#define EVT_MUTE_REQ        ((EventBits_t)(1U << 5))   /* nguoi dung giu nut RESET 2 s        */
#define EVT_ALARM_MUTED     ((EventBits_t)(1U << 6))   /* coi bao dong dang bi tat tieng      */

/**
 * @brief Tao tat ca queue va event group. Goi 1 lan trong main(),
 *        TRUOC khi tao task va truoc vTaskStartScheduler().
 */
void app_ipc_init(void);

#endif /* APP_IPC_H */
