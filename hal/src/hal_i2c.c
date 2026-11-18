/*
 * =============================================================================
 *  File      : hal_i2c.c
 *  Tang      : hal
 *  Chuc nang : I2C master chi ghi, polling, boc thu vien SPL (stm32f10x_i2c.c).
 *              Xem mo ta ham public trong hal_i2c.h.
 *
 *  Cau truc file:
 *    PRIVATE (static, prv...) : doc trang thai bus va bao loi, cho su kien,
 *                               khoa Mutex, phuc hoi bus khi loi
 *    PUBLIC                   : hal_i2c_init / hal_i2c_write
 *
 *  Trinh tu 1 lan ghi va "event" cua SPL (I2C_CheckEvent) can cho:
 *    cho bus ranh -> START  -> EV5 : I2C_EVENT_MASTER_MODE_SELECT
 *                 -> dia chi+W -> EV6 : I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED
 *                 -> reg, data -> EV8_2: I2C_EVENT_MASTER_BYTE_TRANSMITTED (moi byte)
 *                 -> STOP
 * =============================================================================
 */
#include "hal_i2c.h"
#include "hal_delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define I2C_BUS_COUNT       2U
#define I2C_BAD_IDX         0xFFU
#define I2C_MAX_SPEED_HZ    400000UL
#define I2C_MAX_TIMEOUT_MS  50000UL     /* tranh tran phep nhan timeout_ms * 1000 */

/* Thong tin tung bus: chi so 0 = I2C1, 1 = I2C2 */
static SemaphoreHandle_t s_mutex[I2C_BUS_COUNT];
static uint32_t          s_speed[I2C_BUS_COUNT];    /* 0 = chua init */

/* ========================================================================== */
/*                                 PRIVATE                                    */
/* ========================================================================== */

/** @brief Doi con tro I2C sang chi so mang. @return 0, 1 hoac I2C_BAD_IDX. */
static uint8_t prvIdx(I2C_TypeDef *i2c)
{
    if (i2c == I2C1) return 0U;
    if (i2c == I2C2) return 1U;
    return I2C_BAD_IDX;
}

/** @brief Cau hinh phan cung I2C (dung chung cho init va phuc hoi loi). */
static void prvHwInit(I2C_TypeDef *i2c, uint32_t speed_hz)
{
    I2C_InitTypeDef ii;

    I2C_Cmd(i2c, DISABLE);
    I2C_StructInit(&ii);
    ii.I2C_Mode                = I2C_Mode_I2C;
    ii.I2C_DutyCycle           = I2C_DutyCycle_2;
    ii.I2C_OwnAddress1         = 0x00;
    ii.I2C_Ack                 = I2C_Ack_Enable;
    ii.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    ii.I2C_ClockSpeed          = speed_hz;
    I2C_Init(i2c, &ii);
    I2C_Cmd(i2c, ENABLE);
}

/**
 * @brief  DOC TRANG THAI va BAO LOI: kiem tra cac co loi trong thanh ghi SR1.
 *         Co nao bat thi xoa co do va tra ve ma loi tuong ung.
 * @return HAL_I2C_OK neu khong co loi
 *         HAL_I2C_ERR_NACK : co AF   - thiet bi khong ACK
 *         HAL_I2C_ERR_BUS  : co BERR - START/STOP sai vi tri (nhieu tren day)
 *                            co ARLO - mat quyen dieu khien bus
 */
static uint8_t prvCheckError(I2C_TypeDef *i2c)
{
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_AF) == SET) {
        I2C_ClearFlag(i2c, I2C_FLAG_AF);
        return HAL_I2C_ERR_NACK;
    }
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_BERR) == SET) {
        I2C_ClearFlag(i2c, I2C_FLAG_BERR);
        return HAL_I2C_ERR_BUS;
    }
    if (I2C_GetFlagStatus(i2c, I2C_FLAG_ARLO) == SET) {
        I2C_ClearFlag(i2c, I2C_FLAG_ARLO);
        return HAL_I2C_ERR_BUS;
    }
    return HAL_I2C_OK;
}

/**
 * @brief  Tru 1 us vao "ngan sach" thoi gian con lai cua giao dich, cho 1 us.
 * @param  budget_us [in/out] So us con lai, dung chung cho moi buoc cua 1 giao dich
 * @return 1 = het thoi gian, 0 = con thoi gian
 */
static uint8_t prvTick(uint32_t *budget_us)
{
    if (*budget_us == 0U) {
        return 1U;
    }
    (*budget_us)--;
    hal_delay_us(1U);
    return 0U;
}

/**
 * @brief  Cho 1 event cua SPL, dong thoi theo doi co loi va timeout.
 * @param  budget_us [in/out] Thoi gian con lai cua giao dich (us)
 * @return HAL_I2C_OK / ERR_NACK / ERR_BUS / ERR_TIMEOUT
 */
static uint8_t prvWaitEvent(I2C_TypeDef *i2c, uint32_t event, uint32_t *budget_us)
{
    uint8_t err;

    while (I2C_CheckEvent(i2c, event) != SUCCESS) {
        err = prvCheckError(i2c);
        if (err != HAL_I2C_OK) {
            return err;
        }
        if (prvTick(budget_us)) {
            return HAL_I2C_ERR_TIMEOUT;
        }
    }
    return HAL_I2C_OK;
}

/** @brief Cho bus ranh (khong co giao dich nao dang dien ra tren day). */
static uint8_t prvWaitBusFree(I2C_TypeDef *i2c, uint32_t *budget_us)
{
    while (I2C_GetFlagStatus(i2c, I2C_FLAG_BUSY) == SET) {
        if (prvTick(budget_us)) {
            return HAL_I2C_ERR_TIMEOUT;
        }
    }
    return HAL_I2C_OK;
}

/**
 * @brief  Khoa bus bang Mutex. Truoc khi scheduler chay chi co 1 luong nen bo qua.
 * @param  locked [out] 1 = da khoa (phai mo khoa sau khi xong)
 * @return HAL_I2C_OK hoac HAL_I2C_ERR_BUSY
 */
static uint8_t prvLock(uint8_t idx, uint32_t timeout_ms, uint8_t *locked)
{
    *locked = 0U;
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return HAL_I2C_OK;
    }
    if (xSemaphoreTake(s_mutex[idx], pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        return HAL_I2C_ERR_BUSY;
    }
    *locked = 1U;
    return HAL_I2C_OK;
}

/** @brief Phat START, gui dia chi + bit ghi, cho thiet bi ACK. */
static uint8_t prvStartAddr(I2C_TypeDef *i2c, uint8_t addr7, uint32_t *budget_us)
{
    uint8_t st;

    I2C_GenerateSTART(i2c, ENABLE);
    st = prvWaitEvent(i2c, I2C_EVENT_MASTER_MODE_SELECT, budget_us);
    if (st != HAL_I2C_OK) {
        return st;
    }
    I2C_Send7bitAddress(i2c, (uint8_t)(addr7 << 1), I2C_Direction_Transmitter);
    return prvWaitEvent(i2c, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, budget_us);
}

/** @brief Gui 1 byte va cho gui xong. */
static uint8_t prvSendByte(I2C_TypeDef *i2c, uint8_t b, uint32_t *budget_us)
{
    I2C_SendData(i2c, b);
    return prvWaitEvent(i2c, I2C_EVENT_MASTER_BYTE_TRANSMITTED, budget_us);
}

/**
 * @brief  Ket thuc giao dich: phat STOP, va neu loi nang (timeout / loi bus)
 *         thi reset phan cung I2C de lan sau dung duoc.
 */
static void prvFinish(I2C_TypeDef *i2c, uint8_t idx, uint8_t st)
{
    I2C_GenerateSTOP(i2c, ENABLE);

    if (st == HAL_I2C_ERR_TIMEOUT || st == HAL_I2C_ERR_BUS) {
        /* Phuc hoi: STM32F1 hay bi ket co BUSY sau khi bus loi -> reset mem I2C */
        I2C_SoftwareResetCmd(i2c, ENABLE);
        I2C_SoftwareResetCmd(i2c, DISABLE);
        prvHwInit(i2c, s_speed[idx]);
    }
}

/* ========================================================================== */
/*                                  PUBLIC                                    */
/* ========================================================================== */

uint8_t hal_i2c_init(I2C_TypeDef *i2c, uint32_t speed_hz)
{
    uint8_t idx = prvIdx(i2c);

    if (idx == I2C_BAD_IDX || speed_hz == 0U || speed_hz > I2C_MAX_SPEED_HZ) {
        return 1U;
    }

    RCC_APB1PeriphClockCmd((i2c == I2C1) ? RCC_APB1Periph_I2C1 : RCC_APB1Periph_I2C2, ENABLE);
    I2C_DeInit(i2c);
    prvHwInit(i2c, speed_hz);

    if (s_mutex[idx] == NULL) {
        s_mutex[idx] = xSemaphoreCreateMutex();     /* tao truoc scheduler la hop le */
        if (s_mutex[idx] == NULL) {
            return 1U;                              /* het heap FreeRTOS */
        }
    }
    s_speed[idx] = speed_hz;
    return 0U;
}

uint8_t hal_i2c_write(I2C_TypeDef *i2c, uint8_t addr7, uint8_t reg,
                      const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    uint32_t budget_us;                 /* thoi gian con lai cho phan truyen */
    uint8_t  idx = prvIdx(i2c);
    uint8_t  locked, st;
    uint16_t i;

    /* Kiem tra tham so */
    if (idx == I2C_BAD_IDX || s_speed[idx] == 0U || addr7 > 0x7FU ||
        (len > 0U && data == NULL)) {
        return HAL_I2C_ERR_PARAM;
    }
    if (timeout_ms > I2C_MAX_TIMEOUT_MS) {
        timeout_ms = I2C_MAX_TIMEOUT_MS;
    }
    budget_us = timeout_ms * 1000UL;

    st = prvLock(idx, timeout_ms, &locked);
    if (st != HAL_I2C_OK) {
        return st;
    }

    st = prvWaitBusFree(i2c, &budget_us);
    if (st == HAL_I2C_OK) {
        st = prvStartAddr(i2c, addr7, &budget_us);
    }
    if (st == HAL_I2C_OK) {
        st = prvSendByte(i2c, reg, &budget_us);
    }
    for (i = 0U; i < len && st == HAL_I2C_OK; i++) {
        st = prvSendByte(i2c, data[i], &budget_us);
    }

    prvFinish(i2c, idx, st);
    if (locked) {
        xSemaphoreGive(s_mutex[idx]);
    }
    return st;
}
