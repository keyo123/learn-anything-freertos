# USB Composite Device (MSC + CDC) 技术笔记

## 概述

本设备实现 USB 复合设备，同时提供 Mass Storage Class（MSC，U盘）和 Communications Device Class（CDC，虚拟串口）功能。基于 STM32F103RE + STM32 USB 设备库，使用自定义复合类实现（非 ST 默认的单独注册方式）。

## 文件结构

| 文件 | 作用 |
|---|---|
| `USB_DEVICE/App/usbd_composite.c/h` | 复合类核心：注册 MSC + CDC，统一描述符，Setup/DataIn/DataOut 分发 |
| `USB_DEVICE/App/usbd_cdc_if.c/h` | CDC 接口层：数据收发回调、TX 缓冲、回显逻辑 |
| `USB_DEVICE/App/usbd_storage_if.c/h` | MSC 存储接口：W25QXX Flash 的读/写/容量接口 |
| `USB_DEVICE/App/usb_device.c` | USB 设备初始化入口 |
| `USB_DEVICE/App/usbd_desc.c/h` | USB 设备描述符、字符串描述符 |
| `USB_DEVICE/Target/usbd_conf.c` | USB HAL 配置：PMA 缓冲区分配、端点配置、中断注册 |
| `Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c` | ST 官方 CDC 类实现（DataIn/DataOut/Setup 处理） |
| `Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c` | USB 设备核心层（枚举、控制传输、数据分发） |

## USB 描述符结构

### 设备描述符

- bDeviceClass = 0xEF (Misc), bDeviceSubClass = 0x02 (Common), bDeviceProtocol = 0x01 (IAD)
- 使用 IAD 告知 Windows 这是一个复合设备，需要为不同接口加载不同驱动

### 配置描述符布局 (98 bytes)

```
Configuration Descriptor (9 bytes)
├── Interface 0: MSC BOT
│   ├── EP OUT 0x01 (Bulk, 64 bytes)
│   └── EP IN 0x81 (Bulk, 64 bytes)
├── Interface Association Descriptor (IAD)
│   └── bFirstInterface=1, bInterfaceCount=2, Class=CDC
├── Interface 1: CDC Communication
│   ├── Header Functional Descriptor
│   ├── Call Management Functional Descriptor
│   ├── ACM Functional Descriptor
│   ├── Union Functional Descriptor
│   └── EP IN 0x82 (Interrupt, 8 bytes)
└── Interface 2: CDC Data
    ├── EP IN 0x83 (Bulk, 64 bytes)
    └── EP OUT 0x03 (Bulk, 64 bytes)
```

### IAD (Interface Association Descriptor)

IAD 是复合设备的关键。Windows 10+ 依赖 IAD 来正确识别 CDC 接口组：

```c
0x08,                                      // bLength
0x0B,                                      // bDescriptorType: IAD
COMPOSITE_CDC_COMM_INTERFACE,              // bFirstInterface: 1
0x02,                                      // bInterfaceCount: 2 (Comm + Data)
USB_CDC_CLASS,                             // bFunctionClass: 0x02 (CDC)
USB_CDC_COMM_SUBCLASS,                     // bFunctionSubClass: 0x02 (ACM)
USB_CDC_COMM_PROTOCOL,                     // bFunctionProtocol: 0x01 (AT)
0x00,                                      // iFunction
```

## 数据流

### CDC 接收（Host → Device）

```
USB 中断 (USB_LP_CAN1_RX0_IRQHandler)
  → HAL_PCD_IRQHandler
    → HAL_PCD_DataOutStageCallback(hpcd, epnum=3)      // usbd_conf.c
      → USBD_LL_DataOutStage(pdev, epnum=3, ...)        // usbd_core.c:301
        → pdev->pClass->DataOut(pdev, epnum=3)          // usbd_core.c:343
          = Composite_DataOut(pdev, 3)                   // usbd_composite.c:591
            → USBD_CDC_DataOut(pdev, 3)                  // usbd_cdc.c:562
              → itf->pIf_DataRx(hcdc->data, &RxLength)   // usbd_cdc.c:580
                = CDC_DataRx_FS(pbuf, len)               // usbd_cdc_if.c:162
                  → memcpy(g_tx_buf, pbuf, len)          // 缓冲数据
                  → g_tx_pending = 1                     // 标记待发送
```

### CDC 发送回显（Device → Host）

```
main loop (每 5ms)
  → CDC_ProcessTx()                                      // usbd_cdc_if.c:261
    → CDC_Transmit_FS(g_tx_buf, g_tx_len)                // usbd_cdc_if.c:219
      → 临时交换 pClassData 为 CDC handle
      → USBD_CDC_TransmitPacket(&hUsbDeviceFS)            // usbd_cdc.c:612
      → 恢复 pClassData
```

## PMA 缓冲区分配（关键！）

STM32F103 USB PMA 只有 **512 字节**（地址范围 0x000-0x1FF）。PMA_ACCESS=2 时，`HAL_PCDEx_PMAConfig` 的 pmaadress 参数需要乘以 2 得到实际字节偏移。

**当前正确分配**（2026-05-14 修复后）：

| 端点 | pmaadress | 字节偏移 | 大小 | 结束地址 |
|---|---|---|---|---|
| EP0 OUT | 0x20 | 0x40 | 64B | 0x7F |
| EP0 IN | 0x40 | 0x80 | 64B | 0xBF |
| MSC OUT (0x01) | 0x60 | 0xC0 | 64B | 0xFF |
| MSC IN (0x81) | 0x80 | 0x100 | 64B | 0x13F |
| CDC CMD (0x82) | 0xA0 | 0x140 | 8B | 0x147 |
| CDC IN (0x83) | 0xA4 | 0x148 | 64B | 0x187 |
| CDC OUT (0x03) | 0xC4 | 0x188 | 64B | 0x1C7 |

PMA 总使用: 0x1C8 / 0x200 ✓

> **⚠️ 历史教训**: 原始代码中 CDC 端点被分配了 pmaadress=0x118/0x128/0x168，对应的字节偏移（0x230/0x250/0x2D0）全部溢出 0x1FF，导致 USB 硬件访问越界内存，Windows CDC 驱动加载失败报"设备没有发挥作用"。

## MSC + CDC 复合设备的注意事项

### Windows 兼容性

1. **设备描述符必须用 0xEF/0x02/0x01**：告知 Windows 使用 IAD 解析复合设备
2. **IAD 必须紧跟在接口描述符之前**：描述符顺序不能错
3. **bNumInterfaces = 3**：MSC + CDC Communication + CDC Data
4. Windows 10+ 使用 `usbser.sys` 驱动 CDC，无需额外安装驱动
5. CDC ACM 需要 `SET_CONTROL_LINE_STATE` 请求处理（DTR 信号）

### 已知问题

- **首次连接 CDC 失败**：通常由 PMA 溢出或描述符错误导致
- **epnum 始终为 1**：表示只有 MSC 端点在活动，CDC 未被主机识别，应检查描述符和 PMA
- **CDC 无回显**：检查 `CDC_ProcessTx()` 是否在主循环调用，`g_hcdc` 是否已缓存

### 调试方法

1. 在 `HAL_PCD_DataOutStageCallback`、`USBD_LL_DataOutStage`、`Composite_DataOut`、`USBD_CDC_DataOut`、`CDC_DataRx_FS` 逐级打断点
2. 观察 `hUsbDeviceFS.dev_state` 是否等于 `USBD_STATE_CONFIGURED`
3. 检查 `epnum` 值判断数据流向哪个端点
4. 检查 PMA 地址分配是否越界

## 端点地址分配

| 端点 | 地址 | 方向 | 类型 | 用途 |
|---|---|---|---|---|
| MSC OUT | 0x01 | OUT | Bulk | MSC SCSI 命令/数据接收 |
| MSC IN | 0x81 | IN | Bulk | MSC 数据发送 |
| CDC CMD | 0x82 | IN | Interrupt | CDC 串口状态通知 |
| CDC IN | 0x83 | IN | Bulk | CDC 数据发送 |
| CDC OUT | 0x03 | OUT | Bulk | CDC 数据接收 |

## 修改 PMA 后需要同步的地方

如果调整 PMA 地址分配，需要同时修改 `usbd_conf.c` 中的 `HAL_PCDEx_PMAConfig` 调用。无需修改其他文件，因为 PMA 地址通过 PCD 句柄的 `ep->pmaadress` 字段自动传递。
