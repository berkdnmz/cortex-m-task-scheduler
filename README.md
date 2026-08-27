# ARM Cortex-M Bare-Metal Task Scheduler

**English | [🇹🇷 Türkçe](#-türkçe)**

![C](https://img.shields.io/badge/C-Embedded-blue)
![Assembly](https://img.shields.io/badge/Assembly-ARM_Cortex--M4-red)
![STM32F446RE](https://img.shields.io/badge/MCU-STM32F446RE-success)
![Bare Metal](https://img.shields.io/badge/Bare--Metal-Preemptive_Scheduler-orange)

A bare-metal preemptive task scheduler implemented from scratch in Embedded C and ARM Assembly for the STM32F446RE (ARM Cortex-M4).

The project focuses on understanding low-level operating system mechanisms on ARM Cortex-M, including MSP/PSP stack management, task control blocks (TCB), exception handling, context switching, tick-based task blocking, and round-robin task selection without using an external RTOS kernel or STM32 HAL/LL abstractions.

---

## Table of Contents

- [Architectural Overview](#architectural-overview)
- [Key Features](#key-features)
- [Task & Stack Memory Layout](#task--stack-memory-layout)
- [Repository Structure](#repository-structure)
- [Build and Toolchain](#build-and-toolchain)
- [References and Acknowledgments](#references-and-acknowledgments)
- [🇹🇷 Türkçe](#-türkçe)

---

## Architectural Overview

The scheduler leverages core ARM Cortex-M4 hardware mechanisms to implement preemptive multi-tasking:

1. **MSP / PSP Stack Separation**
   - **MSP (Main Stack Pointer):** Used for the scheduler kernel, fault handlers, and interrupt service routines.
   - **PSP (Process Stack Pointer):** Used for task execution.

2. **SysTick and PendSV Coordination**
   - **SysTick:** Generates periodic 1 ms system ticks, updates the global tick counter, checks blocked task wake-up conditions, and triggers a context switch.
   - **PendSV:** Executes at the lowest exception priority to safely perform software context save/restore operations without delaying time-critical ISRs.

3. **Task Control Block (TCB)**
   - Each task maintains an isolated structure storing its current stack pointer (`psp_value`), entry point handler, execution state (`READY`, `BLOCKED`), and wake-up tick.
4. **Tick-Based Task Blocking**
   - Non-blocking `task_delay()` API transitions the active task into the `BLOCKED` state and requests an immediate context switch.
   - The task automatically returns to the `READY` state once its delay count expires.

5. **Round-Robin Task Selection**
   - The scheduler scans task control blocks in circular order to dispatch the next `READY` task.
   - `BLOCKED` tasks are skipped until their delay period completes.
   - If no user tasks are ready, the scheduler falls back to executing the background Idle Task.

6. **Bare-Metal Telemetry & Retargeting**
   - Integrated register-level USART3 BSP driver streaming task execution logs over external USB-to-TTL hardware via standard `printf` (`_write`) retargeting.

---

## Key Features

- **Preemptive Task Scheduling:** Task execution can be preempted through SysTick and PendSV exception mechanisms.
- **Hardware vs. Software Stacking:** Explicit Assembly management for software-saved registers (`R4–R11`) aligned with hardware auto-stacked registers (`xPSR`, `PC`, `LR`, `R12`, `R3–R0`).
- **TCB-Driven Lifecycle:** Isolated per-task stack pointers, handlers, execution states, and non-blocking blocking counters.
- **Cortex-M Exception Return:** Hardware exception return (`0xFFFFFFFD`) utilizing PSP for deterministic task entry.
- **Bare-Metal Telemetry:** Custom USART3 (`PB10`/`PB11`) driver integration for real-time serial execution logging.
- **Comprehensive Fault Handling:** Active handlers for `MemManage`, `BusFault`, `UsageFault`, and `HardFault` for low-level post-mortem debugging.

---

## Task & Stack Memory Layout

```text
SRAM Memory Layout (128 KB Total)
0x20020000  SRAM_END
+---------------------------------------------------+
| Task 1 Stack                                      |
| 1 KB                                              |
| T1_STACK_START = 0x20020000                       |
|                                                   |
|                 ↓ Stack grows downward            |
+---------------------------------------------------+
| Task 2 Stack                                      |
| 1 KB                                              |
| T2_STACK_START = 0x2001FC00                       |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Task 3 Stack                                      |
| 1 KB                                              |
| T3_STACK_START = 0x2001F800                       |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Task 4 Stack                                      |
| 1 KB                                              |
| T4_STACK_START = 0x2001F400                       |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Idle Task Stack                                   |
| 1 KB                                              |
| IDLE_STACK_START = 0x2001F000                     |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Scheduler Stack (MSP)                             |
| 1 KB                                              |
| SCHED_STACK_START = 0x2001EC00                    |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Global / Static Data                              |
| TCBs, .data, .bss, etc.                           |
+---------------------------------------------------+
0x20000000  SRAM_START
```

---

## Repository Structure

```text
.
|-- drivers/
|   |-- Inc/
|   |   |-- stm32f446xx.h
|   |   |-- stm32f446xx_gpio_driver.h
|   |   |-- stm32f446xx_i2c_driver.h
|   |   |-- stm32f446xx_spi_driver.h
|   |   `-- stm32f446xx_usart_driver.h
|   `-- Src/
|       |-- stm32f446xx_gpio_driver.c
|       |-- stm32f446xx_i2c_driver.c
|       |-- stm32f446xx_spi_driver.c
|       `-- stm32f446xx_usart_driver.c
|-- Inc/
|   |-- bsp_uart.h
|   `-- main.h
|-- Src/
|   |-- bsp_uart.c
|   |-- main.c
|   |-- syscalls.c
|   `-- sysmem.c
|-- Startup/
|   `-- startup_stm32f446retx.s
|-- STM32F446RETX_FLASH.ld
|-- STM32F446RETX_RAM.ld
`-- README.md
```

---

## Build and Toolchain

| Item | Value |
|:---|:---|
| **Target MCU** | STM32F446RE |
| **Core** | ARM Cortex-M4 with FPU |
| **Flash** | 512 KB |
| **SRAM** | 128 KB |
| **Board** | NUCLEO-F446RE |
| **IDE** | STM32CubeIDE |
| **Compiler** | arm-none-eabi-gcc (C99 / GNU11, `-O0`) |
| **Debugger** | ST-LINK/V2-1 (SWD) / External USB-to-TTL |

### Build and Run Instructions

1. Clone this repository:
   ```bash
   git clone https://github.com/berkdnmz/cortex-m-task-scheduler.git
   ```
2. Open the project in **STM32CubeIDE**.
3. Connect your NUCLEO-F446RE board.
4. Connect the USB-to-TTL adapter for telemetry:
   - **GND** $\rightarrow$ Nucleo **GND**
   - **RX** $\rightarrow$ Nucleo **PB10 (USART3_TX / D6)**
5. Build the project (`Ctrl + B`).
6. Flash and Debug (`F11` or `Run`).
7. Open a serial terminal (PuTTY / TeraTerm) on the USB-to-TTL COM port at **115200 Baud** (8N1).

---

## References and Acknowledgments

### STMicroelectronics & ARM
- STM32F446xx Reference Manual (RM0390)
- STM32F446xE Datasheet
- ARM Cortex-M4 Generic User Guide (DUI0553)
- ARMv7-M Architecture Reference Manual

### Learning Resource
- FastBit Embedded Brain Academy — Embedded Systems Programming on ARM Cortex-M3/M4 Processor

---

# 🇹🇷 Türkçe

**[🇬🇧 English](#arm-cortex-m-bare-metal-task-scheduler) | Türkçe**

STM32F446RE (ARM Cortex-M4) mikrodenetleyicisi üzerinde **Embedded C** ve **ARM Assembly** kullanılarak sıfırdan geliştirilmiş, preemptive bare-metal görev zamanlayıcı (task scheduler).

Bu projede hazır bir RTOS çekirdeği veya STM32Cube HAL/LL kütüphaneleri kullanılmadan; ARM Cortex-M4'ün MSP/PSP stack yapısı, TCB mimarisi, exception mekanizmaları, context switching, tick tabanlı task blocking ve READY task'ların round-robin mantığıyla seçilmesi donanım seviyesinde gerçeklenmiştir.

---

## İçindekiler

- [Mimari Genel Bakış](#mimari-genel-bakış)
- [Temel Özellikler](#temel-özellikler-1)
- [Bellek ve Yığın (Stack) Yapısı](#bellek-ve-yığın-stack-yapısı)
- [Repository Yapısı](#repository-yapısı-1)
- [Derleme ve Geliştirme Ortamı](#derleme-ve-geliştirme-ortamı-1)
- [Kaynaklar ve Teşekkür](#kaynaklar-ve-teşekkür-1)

---

## Mimari Genel Bakış

Zamanlayıcı, deterministik bir çoklu görev akışı sağlamak için ARM Cortex-M4 mimarisinin donanımsal yeteneklerinden yararlanır:

1. **Stack Pointer İzolasyonu (MSP / PSP)**
   - **MSP (Main Stack Pointer):** Kernel, scheduler yönetimi, donanımsal fault handler'lar ve ISR rutinlerine ayrılmıştır.
   - **PSP (Process Stack Pointer):** Task'ların çalışması sırasında kullanılır.

2. **SysTick ve PendSV Koordinasyonu**
   - **SysTick:** Periyodik 1 ms sistem kesmesi üreterek global tick sayacını günceller, BLOCKED görevlerin uyanma koşullarını kontrol eder ve context switch talebi oluşturur.
   - **PendSV:** En düşük kesme önceliğinde çalışarak yazılımsal register'ları (`R4–R11`) Assembly ile yığına kaydeder, TCB üzerinden stack pointer değişimini yapar ve bir sonraki görevin bağlamını yükler.

3. **Task Control Block (TCB)**
   - Her görev için güncel stack pointer (`psp_value`), giriş fonksiyonu adresi, çalışma durumu (`READY`, `BLOCKED`) ve uyanma tick değeri saklanır.
4. **Tick Tabanlı Görev Bloklama (Task Delay)**
   - `task_delay()` API'si ile işlemciyi meşgul etmeyen (non-blocking) tick tabanlı görev uyutma mekanizması kurulmuştur. Süresi dolan görev otomatik olarak tekrar `READY` olur.

5. **Round-Robin Görev Seçimi**
   - Scheduler, görevleri dairesel sırada tarayarak bir sonraki `READY` görevi seçer. `BLOCKED` görevler atlanır; çalıştırılacak görev kalmadığında arka plandaki Idle Task devreye girer.

6. **Bare-Metal Telemetri ve Loglama**
   - Sıfırdan yazılan bare-metal USART3 BSP sürücüsü ve `_write` retargeting mekanizması ile görev geçişleri harici USB-TTL modülü üzerinden terminale aktarılır.

---

## Temel Özellikler

- **Preemptive Task Scheduling:** SysTick ve PendSV exception mekanizmaları üzerinden görev çalışmasının kesilerek başka bir göreve geçilebilmesi.
- **ARM Assembly ile Context Yönetimi:** Donanım yığını (`xPSR`, `PC`, `LR`, `R12`, `R3–R0`) ve yazılım yığını (`R4–R11`) kayıtlarının Assembly düzeyinde yönetimi.
- **TCB Tabanlı Durum Takibi:** Her görevin stack pointer'ı, çalışma modu ve bekleme sayaçlarının bağımsız yönetilmesi.
- **Gelişmiş Fault Handling:** `MemManage`, `BusFault`, `UsageFault` ve `HardFault` mekanizmaları ile donanım ve yığın hatalarının izlenmesi.
- **Bare-Metal Telemetri:** USART3 (`PB10`/`PB11`) ve `_write` fonksiyon yönlendirmesi ile register seviyesinde `printf` loglama altyapısı.

---

## Bellek ve Yığın (Stack) Yapısı

```text
SRAM Bellek Dağılımı (Toplam 128 KB)
0x20020000  SRAM_END
+---------------------------------------------------+
| Task 1 Stack                                      |
| 1 KB                                              |
| T1_STACK_START = 0x20020000                       |
|                                                   |
|                 ↓ Yığın aşağıya doğru büyür       |
+---------------------------------------------------+
| Task 2 Stack                                      |
| 1 KB                                              |
| T2_STACK_START = 0x2001FC00                       |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Task 3 Stack                                      |
| 1 KB                                              |
| T3_STACK_START = 0x2001F800                       |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Task 4 Stack                                      |
| 1 KB                                              |
| T4_STACK_START = 0x2001F400                       |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Idle Task Stack                                   |
| 1 KB                                              |
| IDLE_STACK_START = 0x2001F000                     |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Scheduler Stack (MSP)                             |
| 1 KB                                              |
| SCHED_STACK_START = 0x2001EC00                    |
|                                                   |
|                 ↓                                 |
+---------------------------------------------------+
| Global / Statik Veri Alanı                        |
| TCB Yapıları, .data, .bss vb.                     |
+---------------------------------------------------+
0x20000000  SRAM_START
```

---

## Repository Yapısı

```text
.
|-- drivers/
|   |-- Inc/
|   |   |-- stm32f446xx.h
|   |   |-- stm32f446xx_gpio_driver.h
|   |   |-- stm32f446xx_i2c_driver.h
|   |   |-- stm32f446xx_spi_driver.h
|   |   `-- stm32f446xx_usart_driver.h
|   `-- Src/
|       |-- stm32f446xx_gpio_driver.c
|       |-- stm32f446xx_i2c_driver.c
|       |-- stm32f446xx_spi_driver.c
|       `-- stm32f446xx_usart_driver.c
|-- Inc/
|   |-- bsp_uart.h
|   `-- main.h
|-- Src/
|   |-- bsp_uart.c
|   |-- main.c
|   |-- syscalls.c
|   `-- sysmem.c
|-- Startup/
|   `-- startup_stm32f446retx.s
|-- STM32F446RETX_FLASH.ld
|-- STM32F446RETX_RAM.ld
`-- README.md
```

---

## Derleme ve Geliştirme Ortamı

| Özellik | Değer |
|:---|:---|
| **Mikrodenetleyici** | STM32F446RE |
| **Çekirdek** | ARM Cortex-M4 with FPU |
| **Flash** | 512 KB |
| **SRAM** | 128 KB |
| **Geliştirme Kartı** | NUCLEO-F446RE |
| **IDE** | STM32CubeIDE |
| **Derleyici** | arm-none-eabi-gcc (C99 / GNU11, `-O0`) |
| **Debugger** | ST-LINK/V2-1 (SWD) / Harici USB-to-TTL |

### Derleme ve Çalıştırma Adımları

1. Repoyu klonlayın:
   ```bash
   git clone https://github.com/berkdnmz/cortex-m-task-scheduler.git
   ```
2. Projeyi **STM32CubeIDE** ile açın.
3. NUCLEO-F446RE kartını bağlayın.
4. Harici USB-to-TTL modülünü bağlayın:
   - **GND** $\rightarrow$ Nucleo **GND**
   - **RX** $\rightarrow$ Nucleo **PB10 (USART3_TX / D6)**
5. Projeyi derleyin (`Ctrl + B`).
6. Karta yükleyin ve debug başlatın (`F11`).
7. Seri terminali (PuTTY / TeraTerm) ilgili COM portu üzerinde **115200 Baud** hızıyla açın.

---

## Kaynaklar ve Teşekkür

### STMicroelectronics & ARM
- STM32F446xx Reference Manual (RM0390)
- STM32F446xE Datasheet
- ARM Cortex-M4 Generic User Guide (DUI0553)
- ARMv7-M Architecture Reference Manual

### Öğrenme Kaynağı
- FastBit Embedded Brain Academy — Embedded Systems Programming on ARM Cortex-M3/M4 Processor