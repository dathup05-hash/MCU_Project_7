/*
 * =============================================================================
 *  File      : app_settings.c
 *  Tang      : app
 *  Chuc nang : Nguong cai dat trong RAM + Mutex. Xem cach dung trong app_settings.h.
 * =============================================================================
 */
#include "FreeRTOS.h"
#include "semphr.h"
#include "app_settings.h"
#include "app_logic.h"

static AppSettings_t     s_cur;             /* cau hinh dang dung  */
static SemaphoreHandle_t s_mutex = NULL;    /* khoa bao ve s_cur   */

void app_settings_init(void)
{
    app_logic_settings_defaults(&s_cur);
    s_mutex = xSemaphoreCreateMutex();
    configASSERT(s_mutex != NULL);
}

void app_settings_get(AppSettings_t *out)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);     /* khoa: cho toi khi lay duoc */
    *out = s_cur;
    xSemaphoreGive(s_mutex);                    /* mo khoa */
}

void app_settings_set(const AppSettings_t *in)
{
    if (!app_logic_settings_is_valid(in)) {
        return;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_cur = *in;
    xSemaphoreGive(s_mutex);
}
