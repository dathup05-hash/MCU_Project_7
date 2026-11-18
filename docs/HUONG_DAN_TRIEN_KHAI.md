# Hướng dẫn triển khai dự án từ đầu đến cuối (bản Keil MDK)

> Dự án: Giám sát nhiệt độ & điều khiển quạt — STM32F103C8T6 + FreeRTOS + SPL
> Theo đúng file `QuyTrinh-Git-2.pdf` của thầy. Code tham chiếu: thư mục `mcu-project/` hiện tại.

Tài liệu này trả lời 4 câu hỏi:
1. Dự án gồm những tầng nào, mỗi tầng có file gì, mỗi file làm gì.
2. Làm theo thứ tự nào (giai đoạn 0 → 3), mỗi bước commit/nhánh git ra sao.
3. Test **từng HAL** và **từng task** trên kit thế nào khi đã có FreeRTOS.
4. Gộp (merge/tích hợp) như thế nào để ra bản release.

---

## 0. Bức tranh tổng thể

### 0.1. Các tầng phần mềm (gọi từ trên xuống)

```
 app/   task_sensor.c, task_control.c ...   <- logic hệ thống, FreeRTOS task
   │      chỉ gọi bsp_xxx(), KHÔNG gọi SPL
   ▼
 bsp/   bsp.c, bsp_ds18b20.c, bsp_oled.c    <- "chân nào nối với cái gì"
   │      gọi hal_xxx()
   ▼
 hal/   hal_gpio.c, hal_i2c.c, hal_pwm.c... <- bọc thư viện SPL, có timeout, thread-safe
   │      gọi GPIO_Init(), I2C_SendData()...
   ▼
 middlewares/STM32F10x_StdPeriph_Driver     <- thư viện của ST (KHÔNG sửa)
 middlewares/CMSIS                          <- startup, system_stm32f10x.c
 middlewares/FreeRTOS                       <- kernel FreeRTOS (KHÔNG sửa)
```

Ví dụ một lệnh đi qua các tầng:
`task_alarm` gọi `bsp_buzzer_set(1)` → bsp biết còi ở PB12 → `hal_gpio_write(GPIOB, GPIO_Pin_12, 1)` → SPL `GPIO_SetBits()` → ghi thanh ghi `BSRR`.

**Tại sao phải chia tầng?** Để 3 người code song song mà không giẫm chân nhau, và đổi board (đổi chân) thì chỉ sửa `bsp.h`.

### 0.2. Toàn bộ file và nhiệm vụ

| Tầng | File | Làm gì | Ai viết |
|---|---|---|---|
| **middlewares** | `CMSIS/startup_stm32f10x_md.s` | Bảng vector ngắt, `Reset_Handler` gọi `SystemInit()` rồi `main()` | Leader chép vào (giai đoạn 0) |
| | `CMSIS/system_stm32f10x.c` | Cấu hình clock 72 MHz | Leader |
| | `STM32F10x_StdPeriph_Driver/` | SPL V3.5.0 (gpio, rcc, i2c, tim, exti, flash, misc) | Leader |
| | `FreeRTOS/` | `tasks.c queue.c list.c event_groups.c timers.c`, `portable/GCC/ARM_CM3/port.c`, `portable/MemMang/heap_4.c` | Leader |
| **cấu hình** | `app/inc/FreeRTOSConfig.h` | Tick 1 kHz, heap 8 KB, ưu tiên ngắt, map `SVC/PendSV/SysTick_Handler` | **Chỉ Leader** |
| | `app/inc/stm32f10x_conf.h` | Chọn module SPL nào được include | Leader |
| **hal** | `hal_types.h` | `hal_status_t`: `HAL_OK / HAL_ERR / HAL_TIMEOUT / HAL_BUSY` | Leader (giai đoạn 0) |
| | `hal_gpio.h/.c` | `hal_gpio_config/write/read/toggle`, `hal_exti_config/enable/disable/clear`, các `EXTIx_IRQHandler` | Dev 1 |
| | `hal_delay.h/.c` | Delay µs bằng bộ đếm DWT: `hal_delay_init/us`, `hal_cycles`, `hal_elapsed_us` | Dev 1 |
| | `hal_i2c.h/.c` | I2C master polling, **có Mutex FreeRTOS**, có timeout: `hal_i2c_init`, `hal_i2c_write_reg` | Dev 2 |
| | `hal_pwm.h/.c` | PWM TIM2/3/4: `hal_pwm_init`, `hal_pwm_set_duty` | Dev 3 |
| | `hal_flash.h/.c` | Xoá page / ghi Flash: `hal_flash_erase_page`, `hal_flash_write` | Dev 3 |
| **bsp** | `bsp.h` | **Bảng chân** (LED PC13, còi PB12, quạt PA6/PB1, OLED PB6/PB7, nút PB13-15, DS18B20 PA1) | Leader |
| | `bsp.c` | `bsp_system_init`, `bsp_io_init`, `bsp_led_set`, `bsp_buzzer_set`, `bsp_fan_set`, nút nhấn | Leader (sau khi HAL merge) |
| | `bsp_ds18b20.h/.c` | Driver 1-Wire DS18B20 | Dev 1 (cùng task-sensor) |
| | `bsp_oled.h/.c`, `bsp_font.h` | Driver SSD1306 qua I2C, framebuffer 1 KB | Dev 2 (cùng task-display) |
| **app** | `app_cfg.h` | **Bảng thông số RTOS**: priority, stack, chu kỳ, ngưỡng mặc định | Cả nhóm họp, Leader commit |
| | `app_ipc.h/.c` | Khai báo + tạo `xTempQueue`, `xFanQueue`, `xButtonQueue`, `xUiEvents`, `g_ui_item` | Leader |
| | `app_logic.h/.c` | Logic thuần (không dính phần cứng/RTOS) → unit test được trên PC | Mỗi dev viết phần của mình |
| | `app_settings.h/.c` | Ngưỡng cài đặt: Mutex + lưu Flash | Dev 3 |
| | `task_sensor.h/.c` | TemperatureTask: đọc DS18B20 mỗi 1 s → `xTempQueue` | Dev 1 |
| | `task_control.h/.c` | ControlTask: mỗi 200 ms tính duty quạt → `bsp_fan_set` + `xFanQueue` | Dev 1 |
| | `task_display.h/.c` | DisplayTask: vẽ OLED 250 ms / khi bấm nút | Dev 2 |
| | `task_input.h/.c` | InputTask: ngắt nút → queue → debounce → sửa ngưỡng | Dev 3 |
| | `task_alarm.h/.c` | AlarmTask: LED + còi quá nhiệt, bíp khi bấm nút | Dev 3 |
| **gốc** | `main.c` | Init → `app_ipc_init` → tạo 5 task → `vTaskStartScheduler()` + 2 hook lỗi | **Chỉ người tích hợp** |
| | `keil/mcu-project.uvprojx` | Project Keil | **Chỉ Leader** |

> So với file mẫu của thầy (có `hal_uart`, `hal_spi`, `hal_adc`): đề tài này không cần UART/SPI/ADC nên 3 nhánh HAL được đổi thành
> `feature/hal-gpio-delay`, `feature/hal-i2c`, `feature/hal-pwm-flash`. Nếu nhóm muốn thêm UART (truyền log lên PC) thì thêm `hal_uart` vào nhánh của Dev 1.

### 0.3. Luồng dữ liệu giữa các task (IPC)

```
 [ngắt EXTI nút] --xButtonQueue--> InputTask --(app_settings + Mutex)--> ngưỡng
                                       │ xUiEvents: EVT_UI_CHANGED, EVT_BEEP
                                       ▼
 SensorTask --xTempQueue (mailbox)--> ControlTask --xFanQueue (mailbox)--> DisplayTask
      │                       ├────> AlarmTask --EVT_ALARM_ACTIVE-------> DisplayTask
      └── EVT_SENSOR_ERROR ───┴────> DisplayTask
```

*Mailbox* = queue 1 phần tử: bên gửi `xQueueOverwrite()` (luôn ghi đè giá trị mới nhất), bên nhận `xQueuePeek()` (đọc mà không lấy ra → nhiều task cùng đọc được).

---

## 1. GIAI ĐOẠN 0 — Leader dựng khung (nhánh `main` → `develop`)

Đây là bước quan trọng nhất: **làm đúng thì 3 người không bao giờ phải sửa file Keil project, không bao giờ conflict ở `main.c`.**

### Bước 0.1 — Tạo repo, `.gitignore`

```bash
cd D:/Project/btl/mcu-project
git init
git branch -M main
```

`.gitignore` (đã có sẵn trong dự án) phải bỏ các file Keil sinh ra:
```
keil/Objects/   keil/Listings/   keil/DebugConfig/
*.uvguix.*   *.uvoptx   *.axf   *.hex   *.o   *.d   *.crf   *.lst   *.map
keil/build_log.txt   unittest/build/   .vscode/
```
> `*.uvoptx` (debug options cá nhân) không commit; `*.uvprojx` (danh sách file, include path, define) **phải** commit.

### Bước 0.2 — Chép middlewares (không tự viết)

- `middlewares/CMSIS/`: `startup_stm32f10x_md.s`, `system_stm32f10x.c/.h`, `stm32f10x.h`
- `middlewares/STM32F10x_StdPeriph_Driver/`: chỉ giữ `misc, rcc, gpio, exti, i2c, tim, flash`
- `middlewares/FreeRTOS/`: kernel + `portable/GCC/ARM_CM3` (vì ARM Compiler 6) + `portable/MemMang/heap_4.c`

### Bước 0.3 — File cấu hình

- `app/inc/stm32f10x_conf.h`: include đúng 7 module SPL trên.
- `app/inc/FreeRTOSConfig.h` — các dòng phải hiểu:

```c
#define configCPU_CLOCK_HZ           (SystemCoreClock)     /* 72 MHz */
#define configTICK_RATE_HZ           ((TickType_t)1000)    /* 1 tick = 1 ms */
#define configMAX_PRIORITIES         6
#define configTOTAL_HEAP_SIZE        ((size_t)(8 * 1024))  /* 8 KB / 20 KB RAM */
#define configCHECK_FOR_STACK_OVERFLOW 2   /* -> vApplicationStackOverflowHook */
#define configUSE_MALLOC_FAILED_HOOK   1   /* -> vApplicationMallocFailedHook  */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
/* => ngắt nào gọi API ...FromISR() phải có priority >= 5 (số lớn = ưu tiên thấp) */

/* Cho FreeRTOS "chiếm" 3 vector ngắt của Cortex-M3 */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

### Bước 0.4 — Thống nhất API (thầy: "Bước 2: Thống nhất API Interface của HAL & IPC")

Leader viết **toàn bộ file `.h`** trước (chỉ có prototype + comment), và **file `.c` dạng stub** (thân rỗng, trả về `HAL_ERR`/0). Ví dụ stub:

```c
/* hal/src/hal_i2c.c  -- STUB, Dev 2 se thay bang code that */
#include "hal_i2c.h"

void hal_i2c_init(I2C_TypeDef *i2c, uint32_t speed_hz)
{
    (void)i2c; (void)speed_hz;
}

hal_status_t hal_i2c_write_reg(I2C_TypeDef *i2c, uint8_t addr7, uint8_t reg,
                               const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    (void)i2c; (void)addr7; (void)reg; (void)data; (void)len; (void)timeout_ms;
    return HAL_ERR;
}
```

Làm stub cho tất cả: `hal_*.c`, `bsp*.c`, `app_logic.c`, `app_settings.c`, `task_*.c` (hàm `task_xxx_create()` trả về `pdPASS`).

**Tại sao stub?**
1. Project Keil có sẵn đủ file ngay từ đầu → Dev không cần thêm file vào `.uvprojx` → không conflict file XML.
2. Build luôn qua (không lỗi `Undefined symbol`), mỗi người chỉ thay thân hàm của mình.

Các header "hợp đồng" quan trọng nhất:
- `hal/inc/hal_types.h` — mã lỗi chung.
- `hal/inc/hal_*.h` — xem mục 2 bên dưới.
- `app/inc/app_cfg.h` — bảng priority/stack/chu kỳ (mục 3.1).
- `app/inc/app_ipc.h` + `app/src/app_ipc.c` — struct `TempData_t`, `FanStatus_t`, các handle queue, các bit `EVT_*`.
  (File thầy để `app_ipc.c` ở giai đoạn 3, nhưng tạo sớm thì các dev test task được ngay; giai đoạn 3 chỉ chỉnh kích thước queue.)
- `app/inc/app_logic.h` — prototype tất cả hàm logic thuần, để mỗi dev chỉ viết phần thân của mình trong `app_logic.c`.

### Bước 0.5 — Tạo project Keil + 2 Target

1. Tạo `keil/mcu-project.uvprojx` theo README (chip STM32F103C8, RTE chỉ tick `CMSIS → CORE`, define `STM32F10X_MD,USE_STDPERIPH_DRIVER`, include path, MicroLIB, ARM Compiler 6).
2. Thêm **Target thứ 2 để test phần cứng**: `Project → Manage → Project Items… → Project Targets` → thêm `TEST_HW`.
3. Tạo thư mục `test_hw/` (xem mục 2.0), thêm vào group `test_hw`.
4. Chọn từng file theo Target (chuột phải file → `Options for File…` → bỏ/tick `Include in Target Build`):

| File | Target `STM32F103C8` (bản thật) | Target `TEST_HW` |
|---|---|---|
| `main.c` | ✔ | ✘ |
| `test_hw/*.c` | ✘ | ✔ |
| tất cả file còn lại | ✔ | ✔ |

Đổi Target bằng ô dropdown trên thanh công cụ Keil.

### Bước 0.6 — Smoke test FreeRTOS (bắt buộc trước khi chia việc)

`main.c` tạm thời của giai đoạn 0 — chỉ để chứng minh FreeRTOS chạy được trên kit:

```c
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f10x.h"

static void prvBlink(void *pv)
{
    GPIO_InitTypeDef gi;
    (void)pv;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    gi.GPIO_Pin = GPIO_Pin_13; gi.GPIO_Mode = GPIO_Mode_Out_PP; gi.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gi);
    for (;;) {
        GPIOC->ODR ^= GPIO_Pin_13;
        vTaskDelay(pdMS_TO_TICKS(500));     /* LED nhay 1 Hz = scheduler + SysTick OK */
    }
}

int main(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    xTaskCreate(prvBlink, "BLINK", 128, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t t, char *n) { (void)t; (void)n; for (;;) {} }
void vApplicationMallocFailedHook(void) { for (;;) {} }
```

F7 build → F8 nạp. LED PC13 nháy đều 0.5 s/lần ⇒ clock 72 MHz, SysTick, PendSV, heap đều đúng.
Nếu không nháy: xem bảng "Lỗi thường gặp" trong `README.md`.

### Bước 0.7 — Commit khung, tạo `develop`

```bash
git add .
git commit -m "chore: project skeleton (SPL, CMSIS, FreeRTOS, Keil project, HAL/APP API stubs)"
git remote add origin <url-repo>
git push -u origin main
git checkout -b develop
git push -u origin develop
```

Trên GitHub: Settings → Branches → bảo vệ `main` và `develop` (bắt buộc Pull Request, không push thẳng).

---

## 2. GIAI ĐOẠN 1 — Code HAL song song

### 2.0. Cách test HAL trên kit mà không đụng `main.c`

Mỗi file test có `main()` riêng nằm trong `test_hw/`, chỉ build ở Target `TEST_HW`.
Vì có nhiều file test mà chỉ được 1 `main()`, mỗi file được bọc bằng `#if`:

`test_hw/test_select.h` (mỗi người sửa số này **trên máy mình**, không commit thay đổi):
```c
#ifndef TEST_SELECT_H
#define TEST_SELECT_H
/* 1=GPIO  2=DELAY  3=I2C  4=PWM  5=FLASH  11=SENSOR 12=CONTROL 13=ALARM 14=INPUT 15=DISPLAY */
#define HW_TEST   1
#endif
```

`test_hw/test_common.c` (Leader tạo ở giai đoạn 0) — vì `main.c` bị loại khỏi Target TEST_HW nên 2 hook FreeRTOS phải có ở đây, không thì lỗi link:
```c
#include "FreeRTOS.h"
#include "task.h"
volatile uint32_t g_test_fail = 0;      /* xem trong Watch: != 0 la co loi */

void vApplicationStackOverflowHook(TaskHandle_t t, char *n) { (void)t; (void)n; g_test_fail = 0xDEAD0001; for (;;) {} }
void vApplicationMallocFailedHook(void)                     { g_test_fail = 0xDEAD0002; for (;;) {} }
```

**Cách xem kết quả không cần UART:** Keil Debug (Ctrl+F5) → `View → Watch Windows → Watch 1` → gõ tên biến `volatile` toàn cục (vd `g_st`, `g_cnt`) → chuột phải → bật `View → Periodic Window Update` → biến cập nhật trực tiếp khi chip đang chạy.

### 2.1. Dev 1 — `feature/hal-gpio-delay`

```bash
git checkout develop
git pull origin develop
git checkout -b feature/hal-gpio-delay
```

**Viết `hal_delay.c`** — bộ đếm DWT (lõi ARM, SPL không có hàm nên ghi thanh ghi trực tiếp):
- `hal_delay_init()`: bật bit TRCENA trong DEMCR (0xE000EDFC), reset CYCCNT (0xE0001004), bật CYCCNTENA.
- `hal_delay_us(us)`: chờ bận đến khi `CYCCNT - start >= us * 72`. Phép trừ không dấu vẫn đúng khi tràn.
- **Chỉ dùng cho µs** (1-Wire, timeout I2C). Chờ ms trong task phải dùng `vTaskDelay()`.

**Viết `hal_gpio.c`**:
- `hal_gpio_config()`: bật clock port (`RCC_APB2PeriphClockCmd`), map enum `HAL_GPIO_xxx` → `GPIO_Mode_xxx`, mode AF thì bật thêm clock AFIO, gọi `GPIO_Init`.
- `hal_gpio_write()`: dùng `GPIO_SetBits/ResetBits` (ghi BSRR/BRR → nguyên tử, an toàn khi nhiều task dùng chung port).
- `hal_exti_config()`: 3 bước — `GPIO_EXTILineConfig` nối chân vào line N → `EXTI_Init` (cạnh kích) → `NVIC_Init` (priority ≥ 5!). Lưu callback vào mảng `s_exti_cb[16]`.
- `EXTI15_10_IRQHandler()` ... : duyệt line nào có cờ thì xoá cờ rồi gọi callback (3 nút PB13/14/15 dùng chung vector này).

**Test trên kit** — `test_hw/test_hal_gpio.c`:
```c
#include "test_select.h"
#if HW_TEST == 1
#include "hal_gpio.h"
#include "hal_delay.h"

volatile uint32_t g_irq_cnt = 0;        /* Watch: tang 1 moi lan bam nut PB13 */

static void prvBtnCb(uint16_t pin)      /* chay TRONG NGAT */
{
    (void)pin;
    g_irq_cnt++;
    hal_gpio_toggle(GPIOB, GPIO_Pin_12);   /* coi keu/tat moi lan bam */
}

int main(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    hal_delay_init();

    hal_gpio_config(GPIOC, GPIO_Pin_13, HAL_GPIO_OUT_PP);       /* LED   */
    hal_gpio_config(GPIOB, GPIO_Pin_12, HAL_GPIO_OUT_PP);       /* coi   */
    hal_gpio_config(GPIOB, GPIO_Pin_13, HAL_GPIO_IN_PULLUP);    /* nut SET */
    hal_exti_config(GPIOB, GPIO_Pin_13, HAL_EXTI_FALLING, 6, prvBtnCb);

    for (;;) {
        hal_gpio_toggle(GPIOC, GPIO_Pin_13);
        hal_delay_us(500000);           /* test luon hal_delay: LED doi trang thai moi 0.5 s */
    }
}
#endif
```
Tiêu chí PASS:
- LED nháy: 10 lần sáng mất đúng ~10 s (bấm đồng hồ điện thoại) ⇒ `hal_delay_us` đúng.
- Bấm nút PB13: còi bật/tắt, `g_irq_cnt` tăng ⇒ GPIO input + EXTI + dispatch đúng.
- `hal_gpio_read(GPIOB, GPIO_Pin_13)` = 1 khi thả, 0 khi bấm (thêm vào Watch nếu muốn).

```bash
git add hal/ test_hw/test_hal_gpio.c
git commit -m "feat(hal): implement gpio, exti dispatch and dwt us-delay"
git push origin feature/hal-gpio-delay
```
→ Mở PR `feature/hal-gpio-delay → develop`.

### 2.2. Dev 2 — `feature/hal-i2c`

**Viết `hal_i2c.c`** — điểm quan trọng về RTOS:
- `hal_i2c_init()`: bật clock I2C, `I2C_Init` (master, 7 bit, 400 kHz), **tạo Mutex** `xSemaphoreCreateMutex()` (tạo trước scheduler là hợp lệ).
- `hal_i2c_write_reg()`:
  1. Nếu scheduler đã chạy → `xSemaphoreTake(mutex, timeout)`; không lấy được → `HAL_BUSY`. (Trước scheduler chỉ có 1 luồng nên không khoá.)
  2. Chuỗi: chờ bus rảnh → START → địa chỉ+W → byte `reg` → `data[]` → STOP. **Mỗi bước chờ đều có timeout** (đo bằng `hal_elapsed_us`), không bao giờ `while(...)` vô hạn.
  3. NACK ở bước địa chỉ → `HAL_ERR` (sai địa chỉ / chưa cắm).
  4. Luôn STOP và `xSemaphoreGive` ở nhãn `out:`.
- Phụ thuộc: cần `hal_delay` (của Dev 1). Lúc đang làm song song thì `hal_delay.c` vẫn là stub → **rebase lên develop sau khi Dev 1 merge** rồi mới test (mục 3.4).

**Test trên kit** — `test_hw/test_hal_i2c.c` (cắm OLED vào PB6/PB7):
```c
#include "test_select.h"
#if HW_TEST == 3
#include "FreeRTOS.h"
#include "task.h"
#include "hal_gpio.h"
#include "hal_delay.h"
#include "hal_i2c.h"

volatile hal_status_t g_st_ok, g_st_wrong;   /* Watch */
volatile uint32_t g_t1 = 0, g_t2 = 0;        /* so lan ghi thanh cong cua 2 task */

static const uint8_t s_on[]  = { 0x8D, 0x14, 0xAF };    /* bat charge pump + display ON */

/* 2 task cung ghi 1 bus -> kiem tra Mutex: khong task nao bi HAL_ERR/TIMEOUT */
static void prvT1(void *pv) { (void)pv; for (;;) { if (hal_i2c_write_reg(I2C1, 0x3C, 0x00, s_on, 3, 50) == HAL_OK) g_t1++; vTaskDelay(1); } }
static void prvT2(void *pv) { (void)pv; for (;;) { if (hal_i2c_write_reg(I2C1, 0x3C, 0x00, s_on, 3, 50) == HAL_OK) g_t2++; vTaskDelay(1); } }

int main(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    hal_delay_init();
    hal_gpio_config(GPIOB, GPIO_Pin_6 | GPIO_Pin_7, HAL_GPIO_AF_OD);
    hal_i2c_init(I2C1, 400000);

    /* Test 1: truoc scheduler (khong khoa mutex) */
    g_st_ok    = hal_i2c_write_reg(I2C1, 0x3C, 0x00, s_on, 3, 50);  /* mong doi HAL_OK  */
    g_st_wrong = hal_i2c_write_reg(I2C1, 0x50, 0x00, s_on, 3, 50);  /* mong doi HAL_ERR */

    /* Test 2: 2 task tranh bus */
    xTaskCreate(prvT1, "T1", 128, NULL, 2, NULL);
    xTaskCreate(prvT2, "T2", 128, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
#endif
```
PASS khi: `g_st_ok == HAL_OK (0)`, `g_st_wrong == HAL_ERR (1)` (không treo!), màn OLED bật (nhiễu điểm ngẫu nhiên là bình thường), `g_t1` và `g_t2` cùng tăng đều. Rút dây SDA lúc đang chạy → chương trình không treo, sẽ trả `HAL_TIMEOUT`/`HAL_ERR`.

```bash
git add hal/src/hal_i2c.c hal/inc/hal_i2c.h test_hw/test_hal_i2c.c
git commit -m "feat(hal): implement thread-safe i2c master with timeout"
```

### 2.3. Dev 3 — `feature/hal-pwm-flash`

**Viết `hal_pwm.c`**:
- Công thức: `f = clk / ((PSC+1)(ARR+1))`, `duty = CCR / (ARR+1)`.
- Chọn PSC nhỏ nhất để ARR ≤ 65535 → độ phân giải duty mịn nhất. Với 72 MHz, 25 kHz: PSC = 0, ARR = 2879.
- Clock timer APB1 = PCLK1 × 2 khi prescaler APB1 ≠ 1 (hàm `prvTimClock`).
- Bật preload (`TIM_OCxPreloadConfig`, `TIM_ARRPreloadConfig`) để đổi duty không bị giật xung.

**Viết `hal_flash.c`**: `FLASH_Unlock` → xoá cờ lỗi cũ → `FLASH_ErasePage` / `FLASH_ProgramHalfWord` (ghép 2 byte, little-endian) → `FLASH_Lock`. Độ dài lẻ → `HAL_ERR`.

**Test PWM** — `test_hw/test_hal_pwm.c`:
```c
#include "test_select.h"
#if HW_TEST == 4
#include "hal_gpio.h"
#include "hal_delay.h"
#include "hal_pwm.h"

volatile uint8_t g_duty;

int main(void)
{
    SystemCoreClockUpdate();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    hal_delay_init();
    hal_gpio_config(GPIOA, GPIO_Pin_6, HAL_GPIO_AF_PP);    /* TIM3_CH1 */
    hal_gpio_config(GPIOB, GPIO_Pin_1, HAL_GPIO_OUT_PP);   /* FAN EN   */
    hal_gpio_write(GPIOB, GPIO_Pin_1, 1);
    hal_pwm_init(TIM3, 1, 25000);

    for (;;) {
        for (g_duty = 0; g_duty <= 100; g_duty += 10) {
            hal_pwm_set_duty(TIM3, 1, g_duty);
            hal_delay_us(1000000);      /* moi muc giu 1 s */
        }
    }
}
#endif
```
PASS khi: quạt tăng tốc dần mỗi giây rồi quay lại 0. Không có quạt: đo đồng hồ vạn năng chân PA6 ≈ `3.3 V × duty/100`, hoặc cắm LED + trở 330 Ω vào PA6 thấy sáng dần.
Kiểm tra thêm trong Watch / `Peripherals → Timers → TIM3`: `ARR = 2879`, `CCR1 = 2880 × duty/100`.

**Test Flash** — `test_hw/test_hal_flash.c`:
```c
#include "test_select.h"
#if HW_TEST == 5
#include "hal_flash.h"

#define TEST_ADDR  0x0800FC00UL                 /* page cuoi (1 KB) */
volatile hal_status_t g_er, g_wr, g_odd;
volatile uint8_t g_match;

int main(void)
{
    static const uint8_t src[8] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };
    const uint8_t *p = (const uint8_t *)TEST_ADDR;
    uint8_t i;

    g_er  = hal_flash_erase_page(TEST_ADDR);    /* mong doi HAL_OK, vung nho = 0xFF */
    g_wr  = hal_flash_write(TEST_ADDR, src, 8); /* mong doi HAL_OK                  */
    g_odd = hal_flash_write(TEST_ADDR + 8, src, 3); /* do dai le -> HAL_ERR          */

    g_match = 1;
    for (i = 0; i < 8; i++) { if (p[i] != src[i]) g_match = 0; }
    for (;;) {}
}
#endif
```
PASS khi `g_match == 1`; mở `View → Memory 1` gõ `0x0800FC00` thấy `11 22 33 ... 88`. Rút điện cắm lại → dữ liệu vẫn còn.

```bash
git add hal/src/hal_pwm.c hal/src/hal_flash.c hal/inc/hal_pwm.h hal/inc/hal_flash.h test_hw/test_hal_pwm.c test_hw/test_hal_flash.c
git commit -m "feat(hal): implement timer pwm and internal flash erase/write"
```

### 2.4. Review & merge HAL

Mỗi PR, người review kiểm tra (theo file thầy):
- [ ] Đúng prototype đã thống nhất ở `hal/inc/` (không tự đổi tên/tham số).
- [ ] Không có vòng `while` chờ phần cứng mà **không có timeout**.
- [ ] Không gọi `vTaskDelay`/API RTOS thường trong callback ngắt; ngắt có priority ≥ 5.
- [ ] Build 0 error, 0 warning ở **cả 2 Target**.

Leader merge lần lượt: `hal-gpio-delay` trước (vì I2C cần `hal_delay`) → `hal-i2c` → `hal-pwm-flash`.

### 2.5. Leader viết `bsp.c` (sau khi 3 HAL đã vào `develop`)

```bash
git checkout develop && git pull origin develop
git checkout -b feature/bsp-board
```
- `bsp_system_init()`: `SystemCoreClockUpdate()`, `NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4)` (FreeRTOS yêu cầu), `hal_delay_init()`.
- `bsp_io_init()`: cấu hình mọi chân, đưa về trạng thái an toàn (LED tắt, còi tắt, quạt 0%), init PWM, init I2C.
- `bsp_led_set` (đảo mức vì LED PC13 sáng mức thấp), `bsp_buzzer_set`, `bsp_fan_set` (duty 0 thì tắt luôn chân EN), các hàm nút nhấn (đổi chân → ID nút).

Test nhanh: Target TEST_HW, file test gọi `bsp_system_init(); bsp_io_init();` rồi lần lượt `bsp_led_set(1)`, `bsp_buzzer_set(1)`, `bsp_fan_set(50)` — quan sát đúng thiết bị. Merge vào `develop`.

---

## 3. GIAI ĐOẠN 2 — Code từng Task FreeRTOS độc lập

### 3.1. Họp chốt `app/inc/app_cfg.h` (trước khi code)

| Task | Priority | Stack (word) | Chu kỳ | Lý do |
|---|---|---|---|---|
| InputTask | 4 | 192 | theo ngắt | Bấm nút phải phản hồi ngay |
| SensorTask | 3 | 160 | 1000 ms | DS18B20 đo mất 750 ms |
| ControlTask | 3 | 160 | 200 ms | Quạt phản ứng nhanh |
| AlarmTask | 2 | 128 | 100 ms | Nháy 2 Hz |
| DisplayTask | 1 | 384 | 250 ms | Chậm vài chục ms không sao; dùng `snprintf` nên stack lớn |

Nguyên tắc: task càng cần phản hồi nhanh càng ưu tiên cao; task tốn thời gian (vẽ OLED ~25 ms) để thấp nhất.
Tổng stack = (192+160+160+128+384) × 4 = 4 KB + Idle 512 B + TCB + queue ≈ 5,5 KB < heap 8 KB.

Nhiệt độ dùng **số nguyên ×10** (`285` = 28,5 °C) vì STM32F103 không có FPU.

### 3.2. Phân công nhánh

| Dev | Nhánh | File |
|---|---|---|
| Dev 1 | `feature/task-sensor` | `bsp_ds18b20.c`, `task_sensor.c` |
| Dev 1 | `feature/task-control` | `task_control.c`, `app_logic_fan_duty/_fan_auto_duty` |
| Dev 2 | `feature/task-display` | `bsp_oled.c`, `bsp_font.h`, `task_display.c`, `app_logic_fmt_x10` |
| Dev 3 | `feature/task-input` | `app_settings.c`, `task_input.c`, `app_logic_settings_*`, `app_logic_ui_next` |
| Dev 3 | `feature/task-alarm` | `task_alarm.c`, `app_logic_alarm_over` |

> `app_settings.c` cần trước cho Control/Alarm/Input → Dev 3 làm & merge `app_settings` sớm nhất có thể (hoặc tách nhánh `feature/app-settings`).

### 3.3. Khuôn mẫu một file task (đúng mẫu của thầy)

```c
/* app/src/task_xxx.c */
#include "FreeRTOS.h"
#include "task.h"
#include "task_xxx.h"
#include "app_ipc.h"
#include "app_cfg.h"
#include "bsp.h"

static TaskHandle_t xXxxTaskHandle = NULL;       /* static: chi file nay thay */

static void prvXxxTask(void *pvParameters)       /* prv = private */
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(TASK_XXX_PERIOD_MS);
    (void)pvParameters;

    /* khoi tao rieng cua task (neu can) */

    for (;;) {
        /* 1. Doc input (queue / event group / bsp) */
        /* 2. Xu ly (goi app_logic_xxx de unit test duoc) */
        /* 3. Xuat ra (bsp / queue / event group) */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);   /* KHONG dung delay_ms() */
    }
}

BaseType_t task_xxx_create(void)                 /* ham DUY NHAT dua ra ngoai */
{
    return xTaskCreate(prvXxxTask, "XXX", TASK_XXX_STACK, NULL,
                       TASK_XXX_PRIO, &xXxxTaskHandle);
}
```

`vTaskDelayUntil` (chu kỳ cố định, không trôi) dùng cho task định kỳ; `vTaskDelay` dùng cho "chờ X ms" (như chờ DS18B20 750 ms).

### 3.4. Test từng task một mình trên kit (không sửa `main.c`)

Ý tưởng: trong `test_hw/test_task_xxx.c`, **chỉ tạo task đang test** + 1 task giả (**dummy**) đóng vai các task còn lại: hoặc *bơm dữ liệu giả vào queue* (thay task gửi), hoặc *đọc queue ra biến Watch* (thay task nhận). Đây là "mock queue / dummy data" mà file thầy nói.

#### a) TemperatureTask — dummy đóng vai người NHẬN

```c
#include "test_select.h"
#if HW_TEST == 11
#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"
#include "app_ipc.h"
#include "task_sensor.h"

volatile int16_t  g_temp_x10;     /* Watch: 285 = 28.5 C   */
volatile uint8_t  g_valid;        /* Watch: 1 = doc duoc    */
volatile uint32_t g_rx_cnt;       /* Watch: tang 1 moi giay */
volatile uint32_t g_heap_free;

static void prvDummyConsumer(void *pv)
{
    TempData_t d;
    (void)pv;
    for (;;) {
        if (xQueuePeek(xTempQueue, &d, pdMS_TO_TICKS(2000)) == pdPASS) {
            g_temp_x10 = d.temp_x10;
            g_valid    = d.valid;
            g_rx_cnt++;
            bsp_led_set(d.valid);       /* LED sang = doc cam bien OK */
        }
        g_heap_free = xPortGetFreeHeapSize();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_system_init();
    bsp_io_init();
    app_ipc_init();
    task_sensor_create();
    xTaskCreate(prvDummyConsumer, "DUMMY", 128, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
#endif
```
PASS: `g_temp_x10` ≈ nhiệt độ phòng ×10, cầm cảm biến bằng tay thấy tăng; rút dây DQ → sau 3 s `g_valid = 0`; `g_test_fail` luôn 0.

#### b) ControlTask — dummy đóng vai NGƯỜI GỬI (cảm biến giả)

```c
#if HW_TEST == 12
/* ... include nhu tren + app_settings.h, task_control.h ... */
volatile uint8_t g_duty;

static void prvFakeSensor(void *pv)
{
    TempData_t d = { 250, 1, 0 };          /* bat dau 25.0 C */
    FanStatus_t f;
    (void)pv;
    for (;;) {
        xQueueOverwrite(xTempQueue, &d);
        if (xQueuePeek(xFanQueue, &f, 0) == pdPASS) g_duty = f.duty;
        d.temp_x10 += 5;                    /* +0.5 C moi giay */
        if (d.temp_x10 > 500) d.temp_x10 = 250;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    bsp_system_init(); bsp_io_init();
    app_settings_init(); app_ipc_init();
    task_control_create();
    xTaskCreate(prvFakeSensor, "FAKE", 128, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
#endif
```
PASS (ngưỡng mặc định 28 / 35 / 45 °C): dưới 28 °C quạt tắt (`g_duty = 0`); 28→35 °C tăng tuyến tính 30 %→100 %; từ 35 °C trở lên 100 %. Đổi `d.valid = 0` → quạt 100 % (an toàn khi mất cảm biến).

#### c) AlarmTask — giả nhiệt độ cao + giả bấm nút
Dummy: `xQueueOverwrite(xTempQueue, {460,1})` (46 °C) → LED + còi nháy 2 Hz. Đổi về 440 → vẫn kêu (độ trễ 1 °C), 340 → tắt. Mỗi 3 s gọi `xEventGroupSetBits(xUiEvents, EVT_BEEP)` → nghe bíp 40 ms.

#### d) InputTask — dummy đóng vai DisplayTask/AlarmTask
Dummy chờ `xEventGroupWaitBits(xUiEvents, EVT_UI_CHANGED, pdTRUE, ...)` rồi copy `g_ui_item` và `app_settings_get()` ra biến Watch. Bấm SET → `g_ui_item` đổi lần lượt; UP/DOWN → ngưỡng ±5; giữ UP → tự lặp; reset kit → ngưỡng vẫn giữ (đã lưu Flash).

#### e) DisplayTask — dummy bơm `TempData_t` + `FanStatus_t`
Dummy ghi `xTempQueue` và `xFanQueue` giá trị thay đổi mỗi giây; thỉnh thoảng set `EVT_ALARM_ACTIVE` / `EVT_SENSOR_ERROR` → màn hình phải hiện đúng số, đúng chữ "QUA NHIET"/"LOI CB".

#### Unit test logic trên PC (không cần kit)
Phần tính toán (`app_logic.c`) test bằng Ceedling trong `unittest/` (`run_tests.bat`, hiện 55/55 PASS). Viết logic mới → thêm test vào `unittest/test/test_app_logic.c` trước khi mở PR.

### 3.5. Đồng bộ liên tục với `develop` (rebase)

Mỗi khi ai đó merge vào `develop`, những người còn lại cập nhật nhánh của mình:
```bash
git checkout feature/task-sensor
git fetch origin
git rebase origin/develop
# Neu conflict: mo file, sua, roi
git add <file_da_sua>
git rebase --continue
git push --force-with-lease origin feature/task-sensor
```
Hay conflict ở: `app_logic.c` (nhiều người cùng thêm hàm) → giữ **cả hai** đoạn code; `app_cfg.h` → chỉ sửa sau khi cả nhóm đồng ý.

### 3.6. PR task → `develop`

Checklist review (theo file thầy):
- [ ] Task tạo thành công (`task_xxx_create()` trả về `pdPASS`).
- [ ] Không tràn stack: chạy test ≥ 5 phút, `g_test_fail == 0` (hook không bị gọi).
- [ ] Build 0 warning.
- [ ] Không sửa `main.c`, `FreeRTOSConfig.h`, `.uvprojx`.
- [ ] Trong ISR chỉ `...FromISR()`; trong task không `hal_delay_us` dài (chỉ µs), chờ ms dùng `vTaskDelay`.
- [ ] Không gọi I2C/OLED trong ngắt (vì có Mutex).

Commit message: `feat(task-sensor): read ds18b20 every 1s into xTempQueue`.

---

## 4. GIAI ĐOẠN 3 — Tích hợp (Integration)

### 4.1. Tạo nhánh tích hợp
```bash
git checkout develop
git pull origin develop
git checkout -b integration/system-v1
```

### 4.2. Hoàn thiện `main.c` (chọn Target `STM32F103C8`)

```c
int main(void)
{
    /* 1. Phan cung */
    bsp_system_init();          /* NVIC group 4 + DWT */
    bsp_io_init();              /* tat ca chan, ve trang thai an toan */

    /* 2. IPC (phai TRUOC khi tao task) */
    app_settings_init();        /* doc Flash + tao Mutex */
    app_ipc_init();             /* tao 3 queue + 1 event group */

    /* 3. Task */
    if (task_input_create()   != pdPASS) { prvFatal(); }
    if (task_sensor_create()  != pdPASS) { prvFatal(); }
    if (task_control_create() != pdPASS) { prvFatal(); }
    if (task_alarm_create()   != pdPASS) { prvFatal(); }
    if (task_display_create() != pdPASS) { prvFatal(); }

    /* 4. Scheduler - khong bao gio quay lai neu du heap */
    vTaskStartScheduler();
    prvFatal();
    return 0;
}
```
`prvFatal()` = tắt ngắt, **quạt 100 %**, LED sáng, dừng — trạng thái an toàn khi lỗi. Hai hook `vApplicationStackOverflowHook` và `vApplicationMallocFailedHook` đều gọi `prvFatal()`.

**Thứ tự này bắt buộc**: tạo queue trước task (task dùng queue ngay khi chạy); I2C/OLED init trong task vì cần Mutex khi scheduler đã chạy.

### 4.3. Đo và hiệu chỉnh

Thêm tạm 1 task giám sát (chỉ trên nhánh integration, xoá trước khi release). Cần bật tạm `#define INCLUDE_xTaskGetHandle 1` trong `FreeRTOSConfig.h`:

```c
volatile uint32_t    g_heap_free, g_heap_min;
volatile UBaseType_t g_hwm[5];      /* so WORD stack con trong it nhat cua tung task */

static void prvMonitor(void *pv)
{
    static const char *names[5] = { "INPUT", "SENSOR", "CONTROL", "ALARM", "DISPLAY" };
    uint8_t i;
    (void)pv;
    for (;;) {
        g_heap_free = xPortGetFreeHeapSize();
        g_heap_min  = xPortGetMinimumEverFreeHeapSize();
        for (i = 0; i < 5; i++) {
            g_hwm[i] = uxTaskGetStackHighWaterMark(xTaskGetHandle(names[i]));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```
Chạy đủ mọi kịch bản (bấm nút liên tục, hơ nóng cảm biến, rút cảm biến) 10–15 phút rồi đọc Watch:
- `g_hwm[i]` < 20 word → **tăng** stack task đó trong `app_cfg.h`; > 100 word → có thể giảm để tiết kiệm RAM.
- `g_heap_min` < 512 B → tăng `configTOTAL_HEAP_SIZE`.
- Kiểm tra lỗi ưu tiên: Display (prio 1) và task khác cùng dùng I2C → Mutex FreeRTOS có *priority inheritance* nên không bị đảo ưu tiên. Không task nào được giữ Mutex rồi chờ queue (dễ deadlock).

Mọi sửa lỗi tích hợp commit thẳng lên `integration/system-v1`:
```bash
git commit -am "fix(rtos): increase display stack to 384 words"
```

### 4.4. Kiểm thử hệ thống (checklist chấp nhận)

| # | Kịch bản | Kỳ vọng |
|---|---|---|
| 1 | Bật nguồn | OLED hiện nhiệt độ sau ≤ 1 s, quạt tắt nếu < 28 °C |
| 2 | Hơ nóng cảm biến 28→35 °C | Duty tăng 30 → 100 % |
| 3 | ≥ 45 °C | LED + còi nháy 2 Hz, OLED "QUA NHIET", quạt 100 % |
| 4 | Rút DS18B20 | Sau 3 s: "LOI CB", LED nháy, quạt 100 % |
| 5 | SET/UP/DOWN | Đổi mục, ±0,5 °C, bíp mỗi lần bấm |
| 6 | Không bấm 10 s khi đang cài | Tự thoát và lưu |
| 7 | Rút điện, cắm lại | Ngưỡng vẫn giữ |
| 8 | Chạy 30 phút | Không treo, `g_heap_min` ổn định |

### 4.5. Release

```bash
# 1. Merge integration vao develop
git checkout develop
git merge --no-ff integration/system-v1
git push origin develop

# 2. Merge develop vao main + tag
git checkout main
git merge --no-ff develop
git tag -a v1.0.0 -m "Release v1.0.0: FreeRTOS integrated system stable"
git push origin main --tags

# 3. Xoa nhanh integration
git branch -d integration/system-v1
git push origin --delete integration/system-v1
```
(Nên làm bước 1 và 2 qua Pull Request trên GitHub để có lịch sử review.)

---

## 5. Lộ trình tóm tắt (sơ đồ nhánh)

```
main     ●─────────────────────────────────────────────────────────●  v1.0.0
          \                                                       /
develop    ●──────●──────●──────●──────●──●──●──●──●──────────────●
                 /      /      /      /  /  /  /  /  \            /
 hal-gpio-delay ●      /      /      /  /  /  /  /    \          /
 hal-i2c ──────────── ●      /      /  /  /  /  /      \        /
 hal-pwm-flash ────────────── ●    /  /  /  /  /        \      /
 bsp-board ──────────────────────── ●  /  /  /           \    /
 task-sensor ─────────────────────────●  /  /             \  /
 task-control ───────────────────────────●  /    integration/system-v1
 task-input / task-alarm / task-display ────●
```

| Giai đoạn | Ai | Kết quả để chuyển giai đoạn |
|---|---|---|
| 0. Khung | Leader | LED nháy bằng FreeRTOS; mọi `.h` + stub `.c` đã có; Keil có 2 Target |
| 1. HAL | 3 dev song song | 5 file test HAL PASS trên kit, merge vào `develop`; `bsp.c` xong |
| 2. Task | 3 dev song song | 5 file test task PASS với dummy; unit test PC PASS |
| 3. Tích hợp | 1 người / cả nhóm | Checklist 4.4 PASS, đo heap/stack ổn → tag `v1.0.0` |

## 6. Quy chuẩn bắt buộc (tóm tắt file thầy)

1. Chỉ **1 người** sửa `FreeRTOSConfig.h` và file Keil `.uvprojx`.
2. Trong ISR chỉ dùng API `...FromISR()` + `portYIELD_FROM_ISR()`; ngắt priority ≥ 5.
3. Trong task chờ ms bằng `vTaskDelay()`/`vTaskDelayUntil()`, không `delay_ms()`.
4. Mọi hàm HAL chờ phần cứng phải có timeout.
5. Commit: `feat(task-xxx)`, `feat(hal)`, `fix(rtos)`, `refactor(...)`.
6. Không commit `keil/Objects/`, `Listings/`, `*.uvoptx`, `*.uvguix.*`, `build/`.
