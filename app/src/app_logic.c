/*
 * =============================================================================
 *  File      : app_logic.c
 *  Tang      : app
 *  Chuc nang : Logic thuan cua ung dung (khong FreeRTOS, khong phan cung).
 *              Xem mo ta tung ham trong app_logic.h.
 * =============================================================================
 */
#include <stdio.h>
#include "app_logic.h"
#include "app_cfg.h"

/* ----------------------------- Cai dat ---------------------------------- */

void app_logic_settings_defaults(AppSettings_t *s)
{
    s->t_set_x10 = DEF_T_SET_X10;
}

uint8_t app_logic_settings_is_valid(const AppSettings_t *s)
{
    return (uint8_t)(s->t_set_x10 >= T_MIN_X10 && s->t_set_x10 <= T_MAX_X10);
}

void app_logic_settings_adjust(AppSettings_t *s, int8_t dir)
{
    int32_t v = (int32_t)s->t_set_x10 + (int32_t)dir * T_STEP_X10;

    if (v < T_MIN_X10) v = T_MIN_X10;
    if (v > T_MAX_X10) v = T_MAX_X10;
    s->t_set_x10 = (int16_t)v;
}

/* ------------------------------ Quat ------------------------------------ */

uint8_t app_logic_fan_on(uint8_t has_temp, uint8_t temp_valid, int16_t t_x10,
                         const AppSettings_t *s, uint8_t fan_on)
{
    if (!has_temp) {
        return 0U;                      /* chua doc lan nao */
    }
    if (!temp_valid) {
        return 1U;                      /* mat cam bien -> bat quat cho an toan */
    }
    if (t_x10 >= s->t_set_x10) {
        return 1U;
    }
    if (t_x10 < s->t_set_x10 - FAN_HYST_X10) {
        return 0U;
    }
    return fan_on;                      /* trong vung tre: giu nguyen */
}

/* ---------------------------- Canh bao ---------------------------------- */

uint8_t app_logic_alarm_over(uint8_t prev_over, int16_t t_x10)
{
    if (t_x10 >= ALARM_TEMP_X10) {
        return 1U;
    }
    if (t_x10 < ALARM_TEMP_X10 - ALARM_HYST_X10) {
        return 0U;
    }
    return prev_over;       /* trong vung tre: giu nguyen */
}

/* ---------------------------- Hien thi ---------------------------------- */

void app_logic_fmt_x10(char *buf, size_t n, int16_t v)
{
    int32_t a = (v < 0) ? -(int32_t)v : (int32_t)v;
    snprintf(buf, n, "%s%ld.%ld", (v < 0) ? "-" : "", (long)(a / 10), (long)(a % 10));
}
