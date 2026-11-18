/*
 * =============================================================================
 *  File      : bsp_relay.h
 *  Tang      : bsp
 *  Chuc nang : Relay dong/ngat nguon quat (chan va muc kich xem bsp.h).
 *              Relay la cong tac co khi: chi BAT / TAT, khong dung PWM.
 *              Tang app nen co do tre nhiet (vd bat >= 35 C, tat < 28 C)
 *              de relay khong dong ngat lien tuc.
 * =============================================================================
 */
#ifndef BSP_RELAY_H
#define BSP_RELAY_H

#include <stdint.h>

/** @brief Cau hinh chan relay, de relay o trang thai TAT. Da duoc goi trong bsp_init(). */
void    bsp_relay_init(void);

/** @brief Dong/ngat relay. @param on 1 = dong (quat chay), 0 = ngat (quat tat) */
void    bsp_relay_set(uint8_t on);

/** @brief Trang thai relay lan ghi gan nhat. @return 1 = dang dong, 0 = dang ngat */
uint8_t bsp_relay_get(void);

#endif /* BSP_RELAY_H */
