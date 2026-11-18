/*
 * =============================================================================
 *  File      : app_settings.h
 *  Tang      : app
 *  Chuc nang : Quan ly NGUONG QUAT do nguoi dung chinh bang nut.
 *              Luu trong RAM, co Mutex bao ve (InputTask ghi, cac task khac doc).
 *              Khong luu Flash: mat dien thi quay ve gia tri mac dinh (app_cfg.h).
 *
 *  Cach dung trong task:
 *      AppSettings_t s;
 *      app_settings_get(&s);       // lay ban sao
 *      s.t_set_x10 += 5;           // sua tren ban sao
 *      app_settings_set(&s);       // ghi lai (tu kiem tra hop le)
 * =============================================================================
 */
#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>

/** Bo thong so cai dat */
typedef struct {
    int16_t t_set_x10;      /* nguong quat (x10 do C) */
} AppSettings_t;

/**
 * @brief Nap gia tri mac dinh va tao Mutex.
 *        Goi 1 lan trong main() truoc vTaskStartScheduler().
 */
void app_settings_init(void);

/** @brief Lay ban sao cau hinh hien tai (an toan khi nhieu task cung goi). */
void app_settings_get(AppSettings_t *out);

/**
 * @brief Ghi cau hinh moi. Gia tri KHONG hop le (ngoai 0.0..99.0 C) thi bo qua.
 */
void app_settings_set(const AppSettings_t *in);

#endif /* APP_SETTINGS_H */
