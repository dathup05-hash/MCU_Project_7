# Middlewares

Thư viện bên ngoài, **không sửa code trong này**.

```
middlewares/
├── STM32F10x_StdPeriph_Driver/   # SPL V3.5.0 của ST (inc/ + src/)
│                                 #   chỉ giữ: misc, rcc, gpio, exti, i2c, tim, flash
├── CMSIS/                        # file thiết bị STM32F10x của ST
│   ├── Core/                     #   core_cm3.h + cmsis_*.h (ARM CMSIS 6.4.0, Apache-2.0)
│   ├── stm32f10x.h
│   ├── system_stm32f10x.c/.h     #   clock 72 MHz từ HSE 8 MHz
│   └── startup_stm32f10x_md.s    #   vector ngắt, cú pháp ARM (Keil)
└── FreeRTOS/                     # FreeRTOS Kernel V11.3.3
    ├── include/
    ├── portable/GCC/ARM_CM3/     #   port cho ARM Compiler 6
    ├── portable/RVDS/ARM_CM3/    #   port cho ARM Compiler 5
    ├── portable/MemMang/heap_4.c
    ├── tasks.c  queue.c  list.c  event_groups.c  timers.c
    └── LICENSE.md                #   MIT
```

- Nguồn SPL: bản sao V3.5.0 nguyên gốc của ST trên GitHub (`wajatimur/stm32f10x-stdperiph-lib`).
- Nguồn FreeRTOS: repo chính thức `FreeRTOS/FreeRTOS-Kernel`, tag `V11.3.3`.
- `CMSIS/Core/` chép từ pack ARM::CMSIS 6.4.0 để project tự chứa đủ, không cần RTE.
- `misc.c` đã sửa 1 dòng `NVIC->IP` → `NVIC->IPR` (CMSIS 6 đổi tên thanh ghi).
- `stm32f10x_conf.h` (chọn module SPL) và `FreeRTOSConfig.h` nằm ở `app/inc/`.
- Cần thêm module SPL khác (vd. USART, ADC): chép `.h`/`.c` tương ứng từ SPL V3.5.0 vào đây và bỏ comment dòng `#include` trong `stm32f10x_conf.h`.
