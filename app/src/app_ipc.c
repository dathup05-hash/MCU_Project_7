/*
 * =============================================================================
 *  File      : app_ipc.c
 *  Tang      : app
 *  Chuc nang : Dinh nghia va khoi tao cac doi tuong IPC dung chung.
 *              Xem so do luong du lieu trong app_ipc.h.
 * =============================================================================
 */
#include "app_ipc.h"
#include "app_cfg.h"

QueueHandle_t      xTempQueue = NULL;
QueueHandle_t      xFanQueue  = NULL;
QueueHandle_t      xButtonQueue = NULL;
EventGroupHandle_t xUiEvents  = NULL;

void app_ipc_init(void)
{
    /* Mailbox: queue 1 phan tu, ghi bang xQueueOverwrite, doc bang xQueuePeek */
    xTempQueue = xQueueCreate(1, sizeof(TempData_t));
    xFanQueue  = xQueueCreate(1, sizeof(FanStatus_t));

    /* Hang doi nut bam: moi phan tu la 1 byte ID nut */
    xButtonQueue = xQueueCreate(BUTTON_QUEUE_LEN, sizeof(uint8_t));

    /* Nhom co su kien (toi da 24 bit tren Cortex-M) */
    xUiEvents  = xEventGroupCreate();

    /* Tao that bai (het heap) -> dung lai tai day de de debug */
    configASSERT(xTempQueue != NULL);
    configASSERT(xFanQueue  != NULL);
    configASSERT(xButtonQueue != NULL);
    configASSERT(xUiEvents  != NULL);
}
