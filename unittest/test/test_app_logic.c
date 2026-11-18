/*
 * =============================================================================
 *  File      : test_app_logic.c
 *  Kieu test : Unity thuan (module app_logic khong phu thuoc gi -> khong can mock)
 *  Doi tuong : app/src/app_logic.c
 *
 *  Cau hinh mac dinh dung trong test (app_cfg.h):
 *      nguong thap 28.0 | nguong cao 35.0 | nguong bao 45.0 | AUTO | tay 50%
 * =============================================================================
 */
#include <string.h>
#include "unity.h"
#include "app_logic.h"
#include "app_cfg.h"

static AppSettings_t s;     /* cau hinh dung chung, nap lai truoc moi test */

void setUp(void)
{
    app_logic_settings_defaults(&s);
}

void tearDown(void)
{
}

/* =========================== Duong cong quat AUTO ======================== */

void test_auto_duty_below_low_and_fan_off_is_zero(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, app_logic_fan_auto_duty(279, &s, 0));
    TEST_ASSERT_EQUAL_UINT8(0, app_logic_fan_auto_duty(-100, &s, 0));
}

void test_auto_duty_at_low_threshold_starts_at_min(void)
{
    TEST_ASSERT_EQUAL_UINT8(FAN_DUTY_MIN, app_logic_fan_auto_duty(280, &s, 0));
}

void test_auto_duty_is_linear_between_low_and_high(void)
{
    /* 31.5 C la giua 28.0 va 35.0 -> 30 + 70/2 = 65 % */
    TEST_ASSERT_EQUAL_UINT8(65, app_logic_fan_auto_duty(315, &s, 0));
    TEST_ASSERT_EQUAL_UINT8(99, app_logic_fan_auto_duty(349, &s, 0));
}

void test_auto_duty_at_or_above_high_is_full(void)
{
    TEST_ASSERT_EQUAL_UINT8(100, app_logic_fan_auto_duty(350, &s, 0));
    TEST_ASSERT_EQUAL_UINT8(100, app_logic_fan_auto_duty(800, &s, 1));
}

void test_auto_duty_hysteresis_keeps_fan_running_just_below_low(void)
{
    /* Quat dang chay: 27.5..27.9 van giu duty nho nhat, 27.4 moi tat */
    TEST_ASSERT_EQUAL_UINT8(FAN_DUTY_MIN, app_logic_fan_auto_duty(279, &s, 1));
    TEST_ASSERT_EQUAL_UINT8(FAN_DUTY_MIN, app_logic_fan_auto_duty(275, &s, 1));
    TEST_ASSERT_EQUAL_UINT8(0,            app_logic_fan_auto_duty(274, &s, 1));
}

void test_auto_duty_never_exceeds_100_over_full_range(void)
{
    int16_t t;
    for (t = -550; t <= 1250; t++) {
        TEST_ASSERT_LESS_OR_EQUAL_UINT8(100, app_logic_fan_auto_duty(t, &s, 0));
        TEST_ASSERT_LESS_OR_EQUAL_UINT8(100, app_logic_fan_auto_duty(t, &s, 1));
    }
}

/* ======================== Quyet dinh duty cuoi cung ====================== */

void test_fan_duty_without_data_auto_is_off(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, app_logic_fan_duty(0, 0, 0, &s, 0));
}

void test_fan_duty_without_data_manual_uses_manual_duty(void)
{
    s.mode = FAN_MODE_MANUAL;
    s.manual_duty = 70;
    TEST_ASSERT_EQUAL_UINT8(70, app_logic_fan_duty(0, 0, 0, &s, 0));
}

void test_fan_duty_sensor_error_is_full_even_in_manual(void)
{
    s.mode = FAN_MODE_MANUAL;
    s.manual_duty = 10;
    TEST_ASSERT_EQUAL_UINT8(100, app_logic_fan_duty(1, 0, 250, &s, 0));
}

void test_fan_duty_over_alarm_is_full_even_in_manual(void)
{
    s.mode = FAN_MODE_MANUAL;
    s.manual_duty = 20;
    TEST_ASSERT_EQUAL_UINT8(100, app_logic_fan_duty(1, 1, 450, &s, 0));
}

void test_fan_duty_manual_mode_uses_manual_duty(void)
{
    s.mode = FAN_MODE_MANUAL;
    s.manual_duty = 40;
    TEST_ASSERT_EQUAL_UINT8(40, app_logic_fan_duty(1, 1, 340, &s, 0));
}

void test_fan_duty_auto_mode_follows_curve(void)
{
    TEST_ASSERT_EQUAL_UINT8(65, app_logic_fan_duty(1, 1, 315, &s, 0));
}

/* =========================== Canh bao qua nhiet ========================== */

void test_alarm_turns_on_at_threshold(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, app_logic_alarm_over(0, 449, 450));
    TEST_ASSERT_EQUAL_UINT8(1, app_logic_alarm_over(0, 450, 450));
}

void test_alarm_hysteresis_keeps_on_until_one_degree_below(void)
{
    TEST_ASSERT_EQUAL_UINT8(1, app_logic_alarm_over(1, 445, 450));
    TEST_ASSERT_EQUAL_UINT8(1, app_logic_alarm_over(1, 440, 450));
    TEST_ASSERT_EQUAL_UINT8(0, app_logic_alarm_over(1, 439, 450));
}

void test_alarm_in_hysteresis_band_stays_off_if_was_off(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, app_logic_alarm_over(0, 445, 450));
}

/* ========================= Kiem tra cau hinh hop le ====================== */

void test_defaults_are_valid(void)
{
    TEST_ASSERT_TRUE(app_logic_settings_is_valid(&s));
    TEST_ASSERT_EQUAL_INT16(DEF_T_LOW_X10, s.t_low_x10);
    TEST_ASSERT_EQUAL_UINT8(FAN_MODE_AUTO, s.mode);
}

void test_invalid_when_low_negative(void)
{
    s.t_low_x10 = -5;
    TEST_ASSERT_FALSE(app_logic_settings_is_valid(&s));
}

void test_high_must_be_at_least_one_degree_above_low(void)
{
    s.t_high_x10 = s.t_low_x10 + 9;
    TEST_ASSERT_FALSE(app_logic_settings_is_valid(&s));
    s.t_high_x10 = s.t_low_x10 + 10;
    TEST_ASSERT_TRUE(app_logic_settings_is_valid(&s));
}

void test_alarm_must_not_be_below_high(void)
{
    s.t_alarm_x10 = s.t_high_x10 - 1;
    TEST_ASSERT_FALSE(app_logic_settings_is_valid(&s));
    s.t_alarm_x10 = s.t_high_x10;
    TEST_ASSERT_TRUE(app_logic_settings_is_valid(&s));
}

void test_invalid_alarm_above_max_mode_or_duty(void)
{
    AppSettings_t bad = s;
    bad.t_alarm_x10 = T_MAX_X10 + 1;
    TEST_ASSERT_FALSE(app_logic_settings_is_valid(&bad));

    bad = s;
    bad.mode = 2;
    TEST_ASSERT_FALSE(app_logic_settings_is_valid(&bad));

    bad = s;
    bad.manual_duty = 101;
    TEST_ASSERT_FALSE(app_logic_settings_is_valid(&bad));
}

void test_checksum_changes_when_settings_change(void)
{
    AppSettings_t copy = s;
    uint32_t c1 = app_logic_settings_checksum(&s);

    TEST_ASSERT_EQUAL_UINT32(c1, app_logic_settings_checksum(&copy));
    copy.t_low_x10 += 5;
    TEST_ASSERT_NOT_EQUAL(c1, app_logic_settings_checksum(&copy));
}

/* ======================== Chinh nguong bang 3 nut ======================== */

void test_adjust_low_up_and_down_by_half_degree(void)
{
    app_logic_settings_adjust(&s, UI_T_LOW, +1);
    TEST_ASSERT_EQUAL_INT16(285, s.t_low_x10);
    app_logic_settings_adjust(&s, UI_T_LOW, -1);
    app_logic_settings_adjust(&s, UI_T_LOW, -1);
    TEST_ASSERT_EQUAL_INT16(275, s.t_low_x10);
}

void test_adjust_low_cannot_reach_high(void)
{
    s.t_low_x10 = 340;      /* cao = 35.0 -> thap toi da 34.0 */
    app_logic_settings_adjust(&s, UI_T_LOW, +1);
    TEST_ASSERT_EQUAL_INT16(340, s.t_low_x10);
}

void test_adjust_low_cannot_go_below_zero(void)
{
    s.t_low_x10 = 0;
    app_logic_settings_adjust(&s, UI_T_LOW, -1);
    TEST_ASSERT_EQUAL_INT16(0, s.t_low_x10);
}

void test_adjust_high_is_bounded_by_low_and_alarm(void)
{
    s.t_high_x10 = 290;     /* = thap + 1.0 */
    app_logic_settings_adjust(&s, UI_T_HIGH, -1);
    TEST_ASSERT_EQUAL_INT16(290, s.t_high_x10);

    s.t_high_x10 = 450;     /* = nguong bao */
    app_logic_settings_adjust(&s, UI_T_HIGH, +1);
    TEST_ASSERT_EQUAL_INT16(450, s.t_high_x10);
}

void test_adjust_alarm_is_bounded_by_high_and_max(void)
{
    s.t_alarm_x10 = T_MAX_X10;
    app_logic_settings_adjust(&s, UI_T_ALARM, +1);
    TEST_ASSERT_EQUAL_INT16(T_MAX_X10, s.t_alarm_x10);

    s.t_alarm_x10 = s.t_high_x10;
    app_logic_settings_adjust(&s, UI_T_ALARM, -1);
    TEST_ASSERT_EQUAL_INT16(s.t_high_x10, s.t_alarm_x10);
}

void test_adjust_mode_toggles_with_up_or_down(void)
{
    app_logic_settings_adjust(&s, UI_MODE, +1);
    TEST_ASSERT_EQUAL_UINT8(FAN_MODE_MANUAL, s.mode);
    app_logic_settings_adjust(&s, UI_MODE, -1);
    TEST_ASSERT_EQUAL_UINT8(FAN_MODE_AUTO, s.mode);
}

void test_adjust_on_main_screen_changes_duty_only_in_manual(void)
{
    app_logic_settings_adjust(&s, UI_NONE, +1);             /* AUTO: khong doi */
    TEST_ASSERT_EQUAL_UINT8(DEF_MANUAL_DUTY, s.manual_duty);

    s.mode = FAN_MODE_MANUAL;
    app_logic_settings_adjust(&s, UI_NONE, +1);
    TEST_ASSERT_EQUAL_UINT8(DEF_MANUAL_DUTY + DUTY_STEP, s.manual_duty);
}

void test_adjust_manual_duty_clamped_to_0_100(void)
{
    s.mode = FAN_MODE_MANUAL;
    s.manual_duty = 100;
    app_logic_settings_adjust(&s, UI_NONE, +1);
    TEST_ASSERT_EQUAL_UINT8(100, s.manual_duty);

    s.manual_duty = 0;
    app_logic_settings_adjust(&s, UI_NONE, -1);
    TEST_ASSERT_EQUAL_UINT8(0, s.manual_duty);
}

void test_any_sequence_of_button_presses_keeps_settings_valid(void)
{
    /* Bam ngau nhien 5000 lan (bo sinh so gia ngau nhien co dinh -> lap lai duoc) */
    uint32_t seed = 12345U;
    int i;
    for (i = 0; i < 5000; i++) {
        seed = seed * 1103515245U + 12345U;
        UiItem_t item = (UiItem_t)((seed >> 16) % UI_ITEM_COUNT);
        int8_t   dir  = ((seed >> 8) & 1U) ? +1 : -1;
        app_logic_settings_adjust(&s, item, dir);
        TEST_ASSERT_TRUE_MESSAGE(app_logic_settings_is_valid(&s), "cau hinh bi sai sau khi bam nut");
    }
}

void test_set_button_cycles_through_menu_and_wraps(void)
{
    UiItem_t it = UI_NONE;
    it = app_logic_ui_next(it); TEST_ASSERT_EQUAL(UI_T_LOW, it);
    it = app_logic_ui_next(it); TEST_ASSERT_EQUAL(UI_T_HIGH, it);
    it = app_logic_ui_next(it); TEST_ASSERT_EQUAL(UI_T_ALARM, it);
    it = app_logic_ui_next(it); TEST_ASSERT_EQUAL(UI_MODE, it);
    it = app_logic_ui_next(it); TEST_ASSERT_EQUAL(UI_NONE, it);
}

/* ========================== Dinh dang so x10 ============================= */

void test_fmt_x10_positive_zero_negative(void)
{
    char buf[12];
    app_logic_fmt_x10(buf, sizeof(buf), 285);  TEST_ASSERT_EQUAL_STRING("28.5", buf);
    app_logic_fmt_x10(buf, sizeof(buf), 0);    TEST_ASSERT_EQUAL_STRING("0.0", buf);
    app_logic_fmt_x10(buf, sizeof(buf), -55);  TEST_ASSERT_EQUAL_STRING("-5.5", buf);
    app_logic_fmt_x10(buf, sizeof(buf), 990);  TEST_ASSERT_EQUAL_STRING("99.0", buf);
}

void test_fmt_x10_small_negative_keeps_minus_sign(void)
{
    char buf[12];
    app_logic_fmt_x10(buf, sizeof(buf), -5);   /* -0.5 C: de mat dau "-" neu lam sai */
    TEST_ASSERT_EQUAL_STRING("-0.5", buf);
}

void test_fmt_x10_truncates_to_buffer_size(void)
{
    char buf[3];
    memset(buf, 'X', sizeof(buf));
    app_logic_fmt_x10(buf, sizeof(buf), 285);
    TEST_ASSERT_EQUAL_STRING("28", buf);       /* khong tran bo dem */
}
