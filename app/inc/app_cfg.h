/*
 * =============================================================================
 *  File      : app_cfg.h
 *  Tang      : app
 *  Chuc nang : BANG THONG SO dung chung cua ca he thong:
 *              uu tien task, kich thuoc stack, chu ky, nguong mac dinh...
 *              Ca nhom thong nhat bang nay TRUOC khi code task.
 *
 *  Don vi nhiet do: so nguyen x10 (goi la "x10")
 *      285 = 28.5 do C,   -55 = -5.5 do C
 *  -> tranh dung float tren chip khong co FPU (STM32F103).
 * =============================================================================
 */
#ifndef APP_CFG_H
#define APP_CFG_H

/* Khong #include "FreeRTOS.h" o day de file nay dung duoc trong unit test.
 * Cac macro TASK_xxx_PRIO dung tskIDLE_PRIORITY chi duoc dung trong file task,
 * ma file task luon #include "FreeRTOS.h" truoc. */

/* -----------------------------------------------------------------------------
 *  Do uu tien task (so CANG LON uu tien CANG CAO). tskIDLE_PRIORITY = 0.
 *
 *  INPUT (4) > SENSOR = CONTROL (3) > ALARM (2) > DISPLAY (1)
 *  - Input cao nhat: bam nut phai phan hoi ngay.
 *  - Display thap nhat: ve OLED mat ~25 ms, cham chut khong sao.
 * -------------------------------------------------------------------------- */
#define TASK_INPUT_PRIO         (tskIDLE_PRIORITY + 4)
#define TASK_SENSOR_PRIO        (tskIDLE_PRIORITY + 3)
#define TASK_CONTROL_PRIO       (tskIDLE_PRIORITY + 3)
#define TASK_ALARM_PRIO         (tskIDLE_PRIORITY + 2)
#define TASK_DISPLAY_PRIO       (tskIDLE_PRIORITY + 1)

/* -----------------------------------------------------------------------------
 *  Kich thuoc stack, don vi WORD (1 word = 4 byte). Vd 160 word = 640 byte.
 *  Task nao dung snprintf (in so ra UART / OLED) thi can stack lon hon.
 *  Bi tran stack -> vApplicationStackOverflowHook() trong main.c.
 * -------------------------------------------------------------------------- */
#define TASK_INPUT_STACK        256     /* co snprintf de in nguong ra UART */
#define TASK_SENSOR_STACK       256     /* co snprintf de in nhiet do ra UART */
#define TASK_CONTROL_STACK      160
#define TASK_ALARM_STACK        160
#define TASK_DISPLAY_STACK      384     /* nhieu snprintf nhat */

/* -----------------------------------------------------------------------------
 *  Chu ky chay cua task (ms)
 * -------------------------------------------------------------------------- */
#define TASK_SENSOR_PERIOD_MS   1000    /* doc nhiet do moi 1 giay                   */
#define DS18B20_CONV_TIME_MS    750     /* DS18B20 can 750 ms de do (12 bit)         */
#define SENSOR_MAX_FAIL         3       /* doc loi 3 lan lien tiep -> bao LOI cam bien */
#define TASK_CONTROL_PERIOD_MS  500     /* xet bat/tat quat moi 500 ms               */
#define TASK_ALARM_TICK_MS      10      /* nhip chay mau keu coi / nhay LED          */
#define TASK_DISPLAY_PERIOD_MS  250     /* ve lai man hinh moi 250 ms                */

/* -----------------------------------------------------------------------------
 *  Nut nhan (dung ngat EXTI canh xuong)
 * -------------------------------------------------------------------------- */
#define BUTTON_QUEUE_LEN        4       /* so lan bam toi da cho trong hang doi      */
#define BTN_DEBOUNCE_MS         50      /* trong ngat: bo qua canh xuong cach lan truoc < 50 ms */
#define BTN_CONFIRM_MS          20      /* trong task: cho 20 ms roi xem nut con bam that khong */
#define BTN_MUTE_HOLD_MS        2000    /* giu nut RESET 2 s -> tat tieng coi bao dong */

/* -----------------------------------------------------------------------------
 *  NGUONG QUAT (1 nguong duy nhat, chinh bang nut UP/DOWN, RESET ve mac dinh).
 *  Khong luu Flash: moi lan cap nguon lai dung gia tri mac dinh.
 *
 *    Quat BAT  khi T >= nguong
 *    Quat TAT  khi T <  nguong - FAN_HYST    (do tre: relay khong dong ngat lien tuc)
 * -------------------------------------------------------------------------- */
#define DEF_T_SET_X10           350     /* 35.0 C: nguong quat mac dinh            */
#define T_MIN_X10               0       /* nguong thap nhat cho phep: 0.0 C        */
#define T_MAX_X10               990     /* nguong cao nhat cho phep: 99.0 C        */
#define T_STEP_X10              5       /* moi lan bam UP/DOWN: +/- 0.5 C          */
#define FAN_HYST_X10            10      /* do tre tat quat: 1.0 C                  */

/* -----------------------------------------------------------------------------
 *  CANH BAO QUA NHIET (CO DINH, khong chinh bang nut)
 *    Bao dong khi T >= 40.0 C, het bao dong khi T < 39.0 C (do tre 1.0 C)
 * -------------------------------------------------------------------------- */
#define ALARM_TEMP_X10          400     /* 40.0 C */
#define ALARM_HYST_X10          10      /* 1.0 C  */

#endif /* APP_CFG_H */
