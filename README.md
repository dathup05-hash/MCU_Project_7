# Giám sát nhiệt độ & điều khiển quạt tự động — STM32F103 + FreeRTOS

Phần cứng: STM32F103C8T6 (Blue Pill), DS18B20, OLED SSD1306 0.96" I2C, quạt DC qua MOSFET, buzzer, LED, **3 nút nhấn** (SET / UP / DOWN).
Toolchain: **Keil MDK 5 (ARM Compiler 6)** + **STM32F10x Standard Peripheral Library** + **FreeRTOS Kernel**.

## Cấu trúc thư mục

```
mcu-project/
├── bsp/                  # Board support: pin mapping + driver thiết bị ngoài
│   ├── bsp.h / bsp.c     #   bảng chân, LED, buzzer, quạt, nút nhấn
│   ├── bsp_ds18b20.*     #   cảm biến DS18B20 (1-Wire)
│   ├── bsp_oled.*        #   OLED SSD1306 (I2C)
│   └── bsp_font.h        #   font 5x7
├── hal/                  # HAL: bọc Standard Peripheral Library
│   ├── inc/              #   hal_gpio.h, hal_i2c.h, hal_pwm.h, hal_flash.h, hal_delay.h
│   └── src/
├── middlewares/
│   ├── STM32F10x_StdPeriph_Driver/   # SPL V3.5.0 của ST (chỉ giữ module dùng tới)
│   ├── CMSIS/            #   stm32f10x.h, system_stm32f10x.c, startup_stm32f10x_md.s
│   └── FreeRTOS/         # FreeRTOS Kernel (tự chép vào, xem bên dưới)
├── app/                  # FreeRTOS Tasks & IPC
│   ├── inc/              #   FreeRTOSConfig.h, stm32f10x_conf.h, app_cfg.h, app_ipc.h, task_*.h...
│   └── src/              #   app_ipc.c, app_settings.c, task_*.c
├── main.c                # Init phần cứng, tạo task, vTaskStartScheduler()
├── .gitignore
└── README.md
```

### Phân lớp

```
app (task_*.c)  ->  bsp (bsp_*.c)  ->  hal (hal_*.c)  ->  SPL (stm32f10x_*.c)  ->  thanh ghi
```

- **SPL** (`middlewares/STM32F10x_StdPeriph_Driver`, code của ST): hàm cấp thấp `GPIO_Init`, `I2C_SendData`, `TIM_OC1Init`... — tầng này **không sửa**.
- **hal/** (nhóm tự viết, gọi hàm SPL): API gọn cho dự án, có timeout, thread-safe (I2C có Mutex). Không biết gì về board.
  Chỉ 2 chỗ bắt buộc truy cập thanh ghi vì SPL không có hàm: bộ đếm DWT (`hal_delay.c`) và bật/tắt 1 line EXTI (`hal_gpio.c`).
- **bsp/**: biết chân nào nối với gì (`bsp.h`), driver DS18B20/OLED. Đổi board thì chỉ sửa ở đây.
- **app/**: các task FreeRTOS, chỉ gọi `bsp_*`, không gọi SPL trực tiếp.

## Sơ đồ chân

| Khối | Chân | Ghi chú |
|---|---|---|
| DS18B20 DQ | PA1 | Điện trở kéo lên 4.7k về 3.3V |
| Quạt PWM | PA6 (TIM3_CH1) | 25 kHz, vào gate MOSFET |
| Quạt EN | PB1 | Bật/tắt mạch công suất |
| OLED SCL / SDA | PB6 / PB7 (I2C1) | Địa chỉ 0x3C |
| Buzzer | PB12 | Mức cao = kêu |
| LED cảnh báo | PC13 | LED trên board, mức thấp = sáng |
| Nút SET / UP / DOWN | PB13 / PB14 / PB15 | Nối xuống GND, kéo lên nội |

## Task FreeRTOS

| Task | Ưu tiên | Stack | Chu kỳ | Việc |
|---|---|---|---|---|
| InputTask | 4 | 192 | theo ngắt | Đọc 3 nút, sửa ngưỡng, lưu Flash |
| TemperatureTask (`task_sensor`) | 3 | 160 | 1 s | Đọc DS18B20 → `xTempQueue` |
| ControlTask | 3 | 160 | 200 ms | Tính duty PWM quạt |
| AlarmTask | 2 | 128 | 100 ms | LED + buzzer quá nhiệt, bíp khi bấm nút |
| DisplayTask | 1 | 384 | 250 ms / khi bấm nút | Vẽ OLED |

IPC: `xTempQueue`, `xFanQueue` (mailbox 1 phần tử), `xButtonQueue` (ISR → task), `xUiEvents` (event group), Mutex cho ngưỡng cài đặt và bus I2C.

## Cài đặt ngưỡng bằng 3 nút

- **SET**: chuyển mục `Bình thường → Ngưỡng thấp → Ngưỡng cao → Ngưỡng báo → Chế độ → Bình thường`. Khi quay về Bình thường thì lưu vào Flash.
- **UP / DOWN**: ±0.5 °C cho ngưỡng, đổi AUTO/TAY ở mục Chế độ. Giữ nút để tự lặp.
- Ở màn hình Bình thường + chế độ TAY: UP/DOWN chỉnh duty quạt ±10%.
- Không bấm 10 s trong lúc cài đặt thì tự thoát và lưu.
- Ràng buộc: `thấp + 1.0 ≤ cao ≤ báo`.

Điều khiển quạt (AUTO): `T < thấp` → tắt (trễ 0.5 °C); `thấp..cao` → tuyến tính 30%..100%; `≥ cao` → 100%. `≥ báo` hoặc mất cảm biến → 100% kèm cảnh báo, kể cả khi ở chế độ TAY.

---

## Đưa vào Keil MDK

SPL V3.5.0, file startup, CMSIS Core và FreeRTOS Kernel V11.3.3 đã nằm sẵn trong `middlewares/`, không cần tải thêm gì.

### Cách nhanh: mở project có sẵn
Mở **`keil/mcu-project.uvprojx`** → **F7** (Build). Đã build thử với Keil MDK 5.43 + ARM Compiler 6.24:
`0 Error(s), 0 Warning(s)` — Code ≈ 20.3 KB (vừa giới hạn 32 KB của MDK-Lite), RAM ≈ 10.5 KB / 20 KB.

Build bằng dòng lệnh:
```
UV4.exe -b keil\mcu-project.uvprojx -j0 -o build_log.txt
```

Project đã chọn sẵn chip **STM32F103C8** (cần pack **Keil::STM32F1xx_DFP 2.4.1**), nạp Flash qua **ST-Link** với thuật toán `STM32F10x_128.FLM`.

Nạp chip (cắm ST-Link vào SWDIO/SWCLK/GND/3.3V của Blue Pill):
- Trong Keil: **F8** (Download). Lần đầu nếu muốn debug: `Options for Target → Debug → Use: ST-Link Debugger`, trong `Settings → Flash Download` tick *Reset and Run*.
- Dòng lệnh: `UV4.exe -f keil\mcu-project.uvprojx -j0 -o flash_log.txt`

> Ghi chú: `middlewares/STM32F10x_StdPeriph_Driver/src/misc.c` đã sửa 1 dòng (`NVIC->IP` → `NVIC->IPR`) cho tương thích CMSIS 6.

Các bước dưới đây chỉ cần khi muốn **tự tạo project mới** từ đầu.

### 1. Cài pack
`Pack Installer` → cài **Keil::STM32F1xx_DFP** (để chọn được chip) và **ARM::CMSIS** (lấy `core_cm3.h`).

### 2. Chọn port FreeRTOS theo compiler
- ARM Compiler **6** (mặc định Keil mới): dùng `portable/GCC/ARM_CM3`.
- ARM Compiler **5**: dùng `portable/RVDS/ARM_CM3` (thay ở cả Group và Include Path bên dưới).

### 3. Tạo project
`Project → New µVision Project` → lưu vào `mcu-project/keil/` → chọn chip **STM32F103C8**.

### 4. Manage Run-Time Environment (RTE)
Chỉ tick **`CMSIS → CORE`** (cung cấp `core_cm3.h`). **Không** tick `Device → Startup` hay `StdPeriph Drivers` vì project đã có sẵn, tick thêm sẽ bị trùng.

### 5. Thêm file vào các Group
| Group | File |
|---|---|
| app | `../main.c`, `../app/src/*.c` |
| bsp | `../bsp/*.c` |
| hal | `../hal/src/*.c` |
| CMSIS | `../middlewares/CMSIS/startup_stm32f10x_md.s`, `../middlewares/CMSIS/system_stm32f10x.c` |
| SPL | `../middlewares/STM32F10x_StdPeriph_Driver/src/*.c` (7 file) |
| FreeRTOS | `tasks.c`, `queue.c`, `list.c`, `event_groups.c`, `timers.c`, `portable/GCC/ARM_CM3/port.c`, `portable/MemMang/heap_4.c` |

### 6. Options for Target
- **Target**: Xtal = `8.0` MHz, tick **Use MicroLIB**, ARM Compiler = **V6**.
- **C/C++ → Define**: `STM32F10X_MD,USE_STDPERIPH_DRIVER`
- **C/C++ → Include Paths**:
  ```
  ..\app\inc;..\bsp;..\hal\inc;..\middlewares\CMSIS;..\middlewares\STM32F10x_StdPeriph_Driver\inc;..\middlewares\FreeRTOS\include;..\middlewares\FreeRTOS\portable\GCC\ARM_CM3
  ```
- **Asm**: để mặc định (armasm, cú pháp ARM) — file startup là cú pháp ARM.
- **Debug**: ST-Link Debugger → Settings → Flash Download → tick *Reset and Run*.

### 7. Build (F7) và nạp (F8)

### Lỗi thường gặp
| Lỗi | Cách sửa |
|---|---|
| `SVC_Handler`/`PendSV_Handler`/`SysTick_Handler` multiply defined | Có file `stm32f10x_it.c` trong project → xoá 3 hàm đó trong file ấy (FreeRTOS đã map sẵn trong `FreeRTOSConfig.h`). |
| `cannot open source input file "FreeRTOS.h"` | Thiếu include path `middlewares\FreeRTOS\include` hoặc chưa chép FreeRTOS. |
| `cannot open source input file "core_cm3.h"` | Chưa tick `CMSIS → CORE` trong RTE. |
| `Please select first the target STM32F10x device` | Thiếu define `STM32F10X_MD` trong C/C++ → Define. |
| Symbol `Reset_Handler` / `SystemInit` multiply defined | Đã tick `Device → Startup` trong RTE → bỏ tick. |
| Treo ở `configASSERT` trong `port.c` | Ngắt EXTI có ưu tiên < 5 hoặc thiếu `NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4)`. |
| OLED không lên | Kiểm tra địa chỉ 0x3C/0x3D; màn 1.3" (SH1106) thì đặt `BSP_OLED_COL_OFFSET 2` trong `bsp.h`. |
| Nhiệt độ luôn `LOI CB` | Thiếu điện trở kéo lên 4.7k ở chân DQ. |

## Unit test (Ceedling + Unity + CMock)

Test chạy **trên máy tính** (gcc), không cần board. Thư mục `unittest/`:

```
unittest/
├── project.yml              # cấu hình Ceedling
├── run_tests.bat / .sh      # chạy toàn bộ test
└── test/
    ├── test_app_logic.c     # Unity thuần: đường cong quạt, cảnh báo, ngưỡng, định dạng số
    ├── test_bsp_ds18b20.c   # CMock: mô phỏng DS18B20 trên dây 1-Wire, giải mã lệnh, CRC
    ├── test_bsp.c           # CMock: bsp gọi đúng hàm hal, đúng chân, đúng mức logic
    └── support/stm32f10x.h  # header GIẢ để hal_*.h biên dịch được trên PC
```

Chạy: nhấp đúp `unittest/run_tests.bat`, hoặc
```
cd unittest
ceedling test:all
```
Kết quả hiện tại: **55/55 PASSED**.

Cài đặt công cụ (1 lần): MSYS2 → `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ruby`, rồi `gem install ceedling` (Ceedling 1.2.2, đã kèm Unity + CMock).

Để test được, logic thuần (không dính FreeRTOS/phần cứng) được tách vào `app/src/app_logic.c`; các task chỉ gọi lại. Khi thêm logic mới, nên viết vào `app_logic.c` kèm test trong `test_app_logic.c`.

## Quy ước đặt tên & comment

Đặt tên theo kiểu FreeRTOS (giống mẫu `task_sensor.c` trong file quy trình git). Comment viết **tiếng Việt không dấu** để hiển thị đúng trong Keil.

| Tiền tố / kiểu | Nghĩa | Ví dụ |
|---|---|---|
| `prv` | hàm **private** của file (luôn kèm `static`) | `prvSensorTask`, `prvAutoDuty` |
| `x` | biến kiểu handle / `BaseType_t` | `xTempQueue`, `xSensorTaskHandle` |
| `v` / `ux` / `pv` / `pc` | (FreeRTOS) hàm void / unsigned / con trỏ void / con trỏ char | `vTaskDelay`, `pvParameters` |
| `s_` | biến `static` cấp file | `s_fb`, `s_mutex` |
| `g_` | biến toàn cục dùng chung nhiều file | `g_ui_item` |
| `hal_xxx()` / `bsp_xxx()` / `app_xxx()` / `task_xxx_create()` | hàm public của tầng tương ứng | `hal_gpio_write`, `bsp_fan_set` |
| `HAL_` / `BSP_` / `TASK_` / `EVT_` / `UI_` (in hoa) | macro / hằng số / enum | `BSP_LED_PIN`, `EVT_BEEP` |
| `..._x10` | nhiệt độ nhân 10 (tránh dùng float) | `t_low_x10 = 285` → 28.5 °C |
| `..._T` | kiểu struct/enum của app | `TempData_t`, `UiItem_t` |

Mỗi file đều có khối comment đầu file (tầng, chức năng). Mỗi hàm có `@brief` / `@param` / `@return`.

## Quy chuẩn RTOS
- Trong ISR chỉ dùng API `...FromISR()`.
- Trong task dùng `vTaskDelay()` / `vTaskDelayUntil()`, không dùng `HAL_Delay()` / `delay_ms()`.
- Không sửa `main.c` khi đang làm trên nhánh `feature/task-*`.
- Chỉ 01 người phụ trách `FreeRTOSConfig.h`.

## Quy tắc commit
- `feat(task-xxx)`: thêm chức năng task / IPC mới.
- `fix(rtos)`: sửa lỗi stack, priority, deadlock, HAL driver.
- `refactor(...)`: tối ưu cấu trúc code.
