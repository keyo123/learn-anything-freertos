# USB Composite Device (MSC + CDC) 开发笔记

## 概述

本工程在 STM32F103RET6 上实现了 USB 复合设备，同时提供：
- **MSC (Mass Storage Class)** — 大容量存储，通过 W25QXX SPI Flash 作为存储介质
- **CDC (Communication Device Class)** — 虚拟串口，通过 USB 与 PC 进行双向数据通信

## 架构

```
USB Host ←→ USB (STM32F103)
              ├── Interface 0: MSC (Mass Storage Class)
              │     ├── EP 0x01 OUT (Bulk, 64 bytes)
              │     └── EP 0x81 IN  (Bulk, 64 bytes)
              │
              └── IAD (Interface Association Descriptor)
                    └── Interface 1+2: CDC (Virtual COM Port)
                          ├── EP 0x82 IN (Interrupt, 8 bytes — 通知端点)
                          ├── EP 0x83 IN (Bulk, 64 bytes — 数据发送)
                          └── EP 0x03 OUT (Bulk, 64 bytes — 数据接收)
```

## 文件清单

### 新增文件

| 文件 | 用途 |
|------|------|
| `Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc/usbd_cdc.h` | CDC 类中间件头文件 |
| `Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c` | CDC 类中间件实现 |
| `USB_DEVICE/App/usbd_cdc_if.h` | CDC 接口层头文件（应用与中间件的桥梁） |
| `USB_DEVICE/App/usbd_cdc_if.c` | CDC 接口层实现 |
| `USB_DEVICE/App/usbd_composite.h` | 复合设备类头文件（MSC + CDC 包装） |
| `USB_DEVICE/App/usbd_composite.c` | 复合设备类实现 |

### 修改文件

| 文件 | 修改内容 |
|------|----------|
| `USB_DEVICE/App/usb_device.c` | 改用 `USBD_Composite` 注册复合设备，替代单 MSC |
| `USB_DEVICE/App/usbd_desc.c` | 设备描述符改为 `0xEF/0x02/0x01`（IAD 复合设备标识） |
| `USB_DEVICE/Target/usbd_conf.c` | 添加 CDC 端点 PMA 配置，扩大静态内存池 |
| `USB_DEVICE/Target/usbd_conf.h` | `USBD_MAX_NUM_INTERFACES` 改为 3 |
| `EIDE/.eide/eide.yml` | 添加新源码文件和 CDC 包含路径 |

## 复合设备类机制

由于 STM32 USB 设备库仅支持注册单个类（`pdev->pClass`），复合设备通过 `usbd_composite.c` 实现一个"包装类"：

```
USBD_Composite 类
  ├── pClassData → USBD_Composite_HandleTypeDef
  │     ├── msc: USBD_MSC_BOT_HandleTypeDef  (MSC 子句柄)
  │     └── cdc: USBD_CDC_HandleTypeDef      (CDC 子句柄)
  │
  └── 分派逻辑：
        ├── Setup()  → 根据 bInterfaceNumber 分派到 MSC (if=0) 或 CDC (if=1,2)
        ├── DataIn() → 根据端点号分派 (0x81→MSC, 0x82/0x83→CDC)
        └── DataOut()→ 根据端点号分派 (0x01→MSC, 0x03→CDC)
```

关键实现细节：在调用子类函数前，临时将 `pdev->pClassData` 和 `pdev->pUserData` 切换到对应子句柄，调用后恢复。

## 端点与 PMA 配置

PMA (Packet Memory Area) 分配（STM32F103 内部 512 字节）：

| 端点 | 方向 | 类型 | PMA 地址（字节偏移） | 大小 |
|------|------|------|---------------------|------|
| 0x00 | OUT | Control | 0x18 (24) | 64 B |
| 0x80 | IN  | Control | 0x58 (88) | 64 B |
| 0x81 | IN  | Bulk (MSC) | 0x98 (152) | 64 B |
| 0x01 | OUT | Bulk (MSC) | 0xD8 (216) | 64 B |
| 0x82 | IN  | Interrupt (CDC Cmd) | 0x118 (280) | 16 B |
| 0x83 | IN  | Bulk (CDC Data) | 0x128 (296) | 64 B |
| 0x03 | OUT | Bulk (CDC Data) | 0x168 (360) | 64 B |

总计使用约 424 字节，在 512 字节 PMA 范围内。

## 使用指南

### 发送数据到 PC

```c
#include "usbd_cdc_if.h"

// 在任何地方调用
uint8_t data[] = "Hello from STM32!\r\n";
CDC_Transmit_FS(data, sizeof(data) - 1);
```

### 接收来自 PC 的数据

在 `USB_DEVICE/App/usbd_cdc_if.c` 的 `CDC_DataRx_FS` 回调函数中处理：

```c
int8_t CDC_DataRx_FS(uint8_t *pbuf, uint32_t *len)
{
    // pbuf 指向接收到的数据, *len 是数据长度
    // 注意：此回调在 USB ISR 上下文中执行，应尽快处理
    // 或将数据放入队列让主循环处理
    
    // 示例：通过 UART4 转发
    HAL_UART_Transmit(&huart4, pbuf, *len, 1000);
    
    return USBD_OK;
}
```

### 控制信号处理

在 `CDC_Control_FS` 中处理控制请求：

```c
int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    switch (cmd)
    {
        case CDC_SET_LINE_CODING:
            // 主机修改串口参数（波特率、停止位等）
            // pbuf 包含 CDC_LineCoding 结构（7字节）
            break;
            
        case CDC_GET_LINE_CODING:
            // 主机查询串口参数
            break;
            
        case CDC_SET_CONTROL_LINE_STATE:
            // DTR/RTS 信号变化
            // pbuf[0] bit0 = DTR present, bit1 = RTS carrier
            break;
    }
    return USBD_OK;
}
```

### 应用层数据流

```
PC 发送数据 → USB OUT EP (0x03) → USBD_CDC_DataOut()
    → CDC_DataRx_FS() 回调 → 用户处理

用户调用 CDC_Transmit_FS() → hcdc->data 缓冲区
    → USBD_CDC_TransmitPacket() → USB IN EP (0x83) → PC 接收
```

## 初始化顺序

```c
MX_USB_DEVICE_Init(void)
{
    USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS);       // 1. 初始化 USB 核心
    USBD_RegisterClass(&hUsbDeviceFS, &USBD_Composite);   // 2. 注册复合类
    USBD_Composite_RegisterStorage(...);                   // 3. 注册存储回调
    USBD_Composite_RegisterCDC(...);                       // 4. 注册 CDC 回调
    USBD_Start(&hUsbDeviceFS);                             // 5. 启动 USB
    // 此时 Composite_Init() 被调用，初始化 MSC + CDC 端点
}
```

## 注意事项

1. **UART4 不受影响**：原有的 UART4 调试输出继续可用，与 USB CDC 相互独立
2. **USB 时钟必须为 48MHz**：当前系统时钟配置正确（HSE 8MHz, PLL x6 = 48MHz）
3. **USB_ENABLE 引脚（PC1）**：在 `main.c` 中已拉低使能 USB
4. **缓冲区共享**：CDC 发送和接收共用 `hcdc->data` 缓冲区（64 字节），发送完成后才能再次调用 `CDC_Transmit_FS`
5. **内存**：复合句柄约 4.3KB，远低于 64KB RAM 总量
6. **MSC 原有功能完全保留**：W25QXX Flash 作为 U 盘使用不受影响

## 添加新功能

如需在复合设备中添加更多功能（如 HID）：

1. 在 `USBD_Composite_HandleTypeDef` 中添加新子句柄
2. 在 `Composite_Init` 中打开新端点
3. 在 `Composite_Setup`/`DataIn`/`DataOut` 中添加分派逻辑
4. 更新配置描述符（注意 `USBD_COMPOSITE_CONFIG_DESC_SIZ`）
5. 添加中间件源文件到 `eide.yml`

## 调试提示

- PC 端使用串口助手连接出现的 COM 口（需要安装 STM32 Virtual COM Port 驱动）
- 发送特定数据看 `CDC_DataRx_FS` 是否被触发
- `CDC_Transmit_FS` 返回 `USBD_BUSY` 表示上次发送尚未完成
- Windows 下首次使用可能需要手动安装驱动（`stmcdc.inf`）
