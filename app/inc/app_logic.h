/*
 * =============================================================================
 *  File      : app_logic.h
 *  Tang      : app
 *  Chuc nang : Cac ham LOGIC THUAN cua ung dung (chi tinh toan):
 *                - chinh / kiem tra nguong quat
 *                - quyet dinh bat/tat quat (relay) co do tre
 *                - doi so x10 thanh chuoi
 *
 *  Module nay KHONG goi FreeRTOS hay phan cung nen:
 *    - cac task goi lai de dung chung 1 logic
 *    - unit test duoc truc tiep tren may tinh
 * =============================================================================
 */
#ifndef APP_LOGIC_H
#define APP_LOGIC_H

#include <stdint.h>
#include <stddef.h>
#include "app_settings.h"

/* ----------------------------- Cai dat ---------------------------------- */

/** @brief Nap gia tri mac dinh (nguong quat DEF_T_SET_X10 trong app_cfg.h). */
void     app_logic_settings_defaults(AppSettings_t *s);

/** @brief Kiem tra cau hinh hop le. @return 1 = hop le, 0 = khong hop le */
uint8_t  app_logic_settings_is_valid(const AppSettings_t *s);

/**
 * @brief Tang/giam nguong quat 0.5 C, giu trong gioi han T_MIN..T_MAX.
 * @param dir +1 = nut UP, -1 = nut DOWN
 */
void     app_logic_settings_adjust(AppSettings_t *s, int8_t dir);

/* ------------------------------ Quat ------------------------------------ */

/**
 * @brief  Quyet dinh bat/tat quat (theo thu tu uu tien):
 *         1. Chua co so lieu      -> TAT
 *         2. Mat cam bien         -> BAT (an toan)
 *         3. T >= nguong          -> BAT
 *         4. T <  nguong - 1.0 C  -> TAT
 *         5. Nam giua             -> giu nguyen trang thai cu (do tre)
 * @param  has_temp   1 = da co it nhat 1 lan doc
 * @param  temp_valid 1 = lan doc gan nhat hop le
 * @param  fan_on     Trang thai quat hien tai
 * @return 1 = bat quat, 0 = tat quat
 */
uint8_t  app_logic_fan_on(uint8_t has_temp, uint8_t temp_valid, int16_t t_x10,
                          const AppSettings_t *s, uint8_t fan_on);

/* ---------------------------- Canh bao ---------------------------------- */

/**
 * @brief  Cap nhat trang thai qua nhiet co do tre (nguong co dinh ALARM_TEMP_X10):
 *         bat khi T >= 40.0 C, chi tat khi T < 39.0 C.
 * @param  prev_over Trang thai truoc do (1 = dang qua nhiet)
 * @return Trang thai moi
 */
uint8_t  app_logic_alarm_over(uint8_t prev_over, int16_t t_x10);

/* ---------------------------- Hien thi ---------------------------------- */

/**
 * @brief Doi so x10 sang chuoi co phan thap phan. Vd 285 -> "28.5", -55 -> "-5.5".
 * @param buf Bo dem ket qua, n = kich thuoc bo dem
 */
void     app_logic_fmt_x10(char *buf, size_t n, int16_t v);

#endif /* APP_LOGIC_H */
