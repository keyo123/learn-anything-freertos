# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

STM32F103RE firmware project — USB Mass Storage + CDC composite device with FatFs, LVGL GUI on LCD, W25QXX SPI Flash storage, and custom json_parser. Built with **EIDE** (Embedded IDE VS Code extension, `cl.eide`) using **ARM Compiler 5 (AC5)** toolchain.

The EIDE project root is `EIDE/`. All source file references in `EIDE/.eide/eide.yml` use `../` relative paths pointing to the parent `USB_Mass/` directory (the STM32CubeMX-generated project tree).

## Build & Flash Commands

All build/flash actions run via VS Code tasks defined in `.vscode/tasks.json`, which call EIDE commands:

- **Build**: `Terminal > Run Task > build` (calls `eide.project.build`)
- **Rebuild**: `Terminal > Run Task > rebuild` (calls `eide.project.rebuild`)
- **Clean**: `Terminal > Run Task > clean` (calls `eide.project.clean`)
- **Flash (upload)**: `Terminal > Run Task > flash` (calls `eide.project.uploadToDevice`)
- **Build + Flash**: `Terminal > Run Task > build and flash` (calls `eide.project.buildAndFlash`)

Output goes to `EIDE/build/USB_Mass/`.

## Project Configuration

- **MCU**: STM32F103RE (Cortex-M3, 512 KB Flash @ 0x08000000, 64 KB RAM @ 0x20000000)
- **Toolchain**: ARM Compiler 5 (AC5), C99 mode with GNU extensions, optimization `-O3`, one ELF section per function
- **Defines**: `USE_HAL_DRIVER`, `STM32F103xE`, `STM32F10X_HD`, `JSON_PARSER_ENABLE=1`
- **Clock**: 48 MHz (HSE 8 MHz ×6 PLL), USB clock from PLL output
- **EIDE project file**: `.eide/eide.yml` (version 4.1) — defines all source files, include paths, toolchain config, and target settings
- **Uploader**: J-Link (SWD, 8 MHz) — configured inside EIDE settings (not in `.vscode/launch.json`)

## Source Tree Architecture

```
USB_Mass/                          # STM32CubeMX-generated root (parent of EIDE/)
├── Core/
│   ├── Src/                       # User application code (HAL-generated + user code)
│   │   ├── main.c                 # Entry point: HW init, FatFs mount, LVGL UI, USB MSC+CDC loop
│   │   ├── stm32f1xx_it.c         # Interrupt handlers (SysTick, DMA, USB, etc.)
│   │   ├── stm32f1xx_hal_msp.c    # HAL MSP initialization (clocks, GPIO, DMA, USB)
│   │   ├── gpio.c / dma.c / spi.c / tim.c / usart.c  # Peripheral init
│   │   └── system_stm32f1xx.c     # CMSIS system init
│   └── Inc/                       # Corresponding headers
├── Drivers/                       # STM32 HAL + CMSIS (CubeMX-generated, read-only)
├── Middlewares/
│   ├── Third_Party/FatFs/         # FatFs R0.14 core (diskio.c, ff.c, ff_gen_drv.c, syscall.c)
│   └── ST/STM32_USB_Device_Library/ # USB device stack (Core + MSC + CDC classes)
├── FATFS/
│   ├── Target/user_diskio.c       # FatFs disk I/O glue: W25QXX over SPI2
│   └── App/fatfs.c                # FatFs init wrapper
├── FatFs_additional/              # Supplemental FatFs sources (ffsystem.c, ffunicode.c)
├── USB_DEVICE/
│   ├── App/
│   │   ├── usb_device.c           # USB device init
│   │   ├── usbd_desc.c            # USB descriptors (VID/PID, strings)
│   │   ├── usbd_storage_if.c      # MSC storage interface → W25QXX
│   │   ├── usbd_cdc_if.c          # CDC interface (UART4 echo/tx)
│   │   └── usbd_composite.c       # MSC+CDC composite registration
│   └── Target/usbd_conf.c         # USB HAL config (PCD)
├── W25QXX/                        # W25Qxx SPI NOR Flash driver (w25qxx.c/.h, memoryManage.c, sys.c)
├── LCD/                           # LCD driver (lcd.c, lcd_init.c) + key scanning (key.c/.h)
├── LVGL/                          # LVGL v8.x (full library in src/, porting in examples/porting/)
├── JSON/                          # Custom json_parser (arena-based, no malloc)
├── cJSON/                         # Legacy cJSON library (no longer used in main code)
├── MDK-ARM/                       # Startup file (startup_stm32f103xe.s) + legacy Keil project files
└── EIDE/                          # This directory — EIDE project
    ├── .eide/eide.yml             # Main project config (version 4.1)
    ├── .eide/files.options.yml    # Per-file compiler option overrides
    ├── .clang-format              # Code formatting rules (Microsoft style, Linux braces)
    └── build/                     # Build output
```

## Key Architecture Notes

- **USB Composite Device**: Enumerates as both MSC (Mass Storage) and CDC (Virtual COM Port). `usbd_composite.c` registers both class interfaces. CDC uses UART4 for data path. MSC storage backed by W25QXX SPI Flash.
- **Storage path**: USB MSC ↔ `usbd_storage_if.c` ↔ FatFs `user_diskio.c` ↔ SPI2 ↔ W25QXX Flash (drive "0:").
- **FatFs**: LFN enabled. `ff_convert` and `ff_wtoupper` implemented in `main.c` for Unicode support with ASCII-only conversion. Drive "0:" is the W25QXX.
- **LVGL**: v8.x, display via `lv_port_disp.c` (SPI1 LCD), input via `lv_port_indev.c` (keys). TIM3 provides the 1ms tick (`lv_tick_inc`). The UI (`create_test_ui()`) and the LVGL task (`lv_timer_handler()`) are currently disabled in `main.c`.
- **json_parser** (in `JSON/`): Custom lightweight parser designed for Cortex-M embedded use — **no dynamic memory allocation**, uses a fixed-size arena passed to `json_parse()`. Parses in-place (modifies input buffer). Supports object, array, string, number, bool, null. See `json_parser.h` for full API. Previously used cJSON, now migrated to this custom parser.
- **main.c** contains all user code inside STM32CubeMX `USER CODE` sections. Key sections: startup init (HAL, clocks, peripherals), FatFs auto-format on first mount, USB D+ pull-up control, UART4 debug output, file browser with button navigation, and JSON file parsing demo.
- **Clang-format**: Microsoft base style with Linux braces, 4-space indent, no tabs, no column limit.

## Code Conventions

- **Comments**: Primary language is 中文 (Chinese). All user-facing comments and inline documentation are in Chinese.
- **CubeMX sections**: All hand-written code lives within `/* USER CODE BEGIN n */` / `/* USER CODE END n */` delimiters — never write code outside these sections in CubeMX-generated files.
- **Debug output**: Uses `HAL_UART_Transmit(&huart4, ...)` via UART4 for printf-style debug. CDC also available via `CDC_Transmit_FS()` after host opens the VCP.
- **Error handling**: `Error_Handler()` disables interrupts and halts (`while(1)`). FatFs operations return `FRESULT` codes checked inline.

## JSON Parser Quick Reference

```c
// Arena-based parsing — no free() needed
static uint8_t json_pool[4096];
json_value_t *root = json_parse(json_str, json_pool, sizeof(json_pool), NULL);
if (!root) { /* error: json_get_error() */ }

// Accessors
json_get_type(v)           // returns json_type_t
json_get_string(v, &len)   // const char*
json_get_number(v)         // double
json_get_bool(v)           // int (0/1)
json_object_get(obj, key)  // json_value_t* (linear scan)
json_array_get(arr, idx)   // json_value_t*
json_array_size(v)         // uint32_t
json_object_size(v)        // uint32_t
```

## Per-File Compiler Options

`files.options.yml` overrides default `-O3` with empty-string flags for specific files (dma.c, tim.c, stm32f1xx_hal_tim.c, stm32f1xx_hal_tim_ex.c), effectively using the toolchain's default optimization for those files.
