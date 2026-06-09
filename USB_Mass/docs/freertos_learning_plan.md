# FreeRTOS 学习计划

> 基于 STM32F103RE + USB_Mass 项目

---

## 目录

- [学习概览](#学习概览)
- [第一阶段：巩固基础 — 理解调度器内部机制](#第一阶段巩固基础--理解调度器内部机制)
- [第二阶段：IPC 进阶 — 同步与通信](#第二阶段ipc-进阶--同步与通信)
- [第三阶段：实时模式 — 定时器和精确周期](#第三阶段实时模式--定时器和精确周期)
- [第四阶段：调试与诊断](#第四阶段调试与诊断)
- [第五阶段：综合实战 — 系统监控仪表盘](#第五阶段综合实战--系统监控仪表盘)
- [附录：关键 API 速查](#附录关键-api-速查)

---

## 学习概览

### 当前已掌握的功能

| 特性 | 使用位置 | 说明 |
|------|----------|------|
| 多任务 + 优先级 | `main.c` | LVGL(2)、CDC_Tx(3)、FILE_SVC(1) |
| 队列 (Queue) | `app_file_service.c` | FATFS 操作序列化 |
| 任务通知 (Task Notification) | `app_file_service.c`、`usbd_cdc_if.c` | ISR→Task、Task→Task 同步 |
| vTaskDelay | `main.c` | LVGL 5ms 周期让出 CPU |
| SysTick 双模式 | `stm32f1xx_it.c` | 启动前后分别服务 HAL 和 RTOS |
| PendSV/SVC 桥接 | `stm32f1xx_it.c` | 汇编尾调用跳转到 port.c |
| 基础配置 | `FreeRTOSConfig.h` | 堆大小、优先级、中断层级 |

### 学习路径时间线

```
第 1-2 周：第一阶段 — 读调度源码 + 栈测量 + 临界区
第 3-4 周：第二阶段 — 互斥量 + 信号量 + 事件组
第 5 周：  第三阶段 — 软件定时器 + xTaskDelayUntil
第 6 周：  第四阶段 — 调试工具链（溢出检测 + 运行时统计）
第 7-8 周：第五阶段 — 综合项目
```

---

## 第一阶段：巩固基础 — 理解调度器内部机制

### 阶段目标

- 理解 FreeRTOS 调度器的核心工作机制（任务状态机、上下文切换流程）
- 掌握任务栈的计算和优化方法
- 理解临界区和中断安全编程模型
- 能独立分析任务切换的性能开销

### 学习内容 1.1：FreeRTOS 任务状态机

#### 理论知识

FreeRTOS 任务有四种状态，状态转移如下图所示：

```
                    ┌──────────┐
                    │  就绪态   │ ←──── 所有可运行但未获得 CPU 的任务
                    │ (Ready)  │
                    └────┬─────┘
                         │
             调度器分配 CPU│
                         ↓
                    ┌──────────┐
            ┌──────│  运行态   │───────┐
            │      │ (Running)│       │
            │      └──────────┘       │
            │                         │
    更高优先级任务抢占            vTaskDelay/等待事件
            │                         │
            ↓                         ↓
    ┌──────────┐              ┌──────────┐
    │  就绪态   │              │  阻塞态   │
    │ (Ready)  │              │(Blocked) │
    │  ← 也可  │              └──────────┘
    └──────────┘
                                     │
                               vTaskSuspend
                                     ↓
                            ┌──────────┐
                            │  挂起态   │
                            │(Suspended)│
                            └──────────┘
```

**关键理解：阻塞态是 FreeRTOS 的灵魂**。一个设计良好的系统中，CPU 大部分时间在执行**空闲任务**，而不是在轮询。每次 `vTaskDelay`、`xQueueReceive` 调用都在做有用的事——它们把 CPU 让给更需要它的任务。

#### 代码阅读作业

阅读 `FreeRTOS/tasks.c`，找到以下函数并理解其实现：

1. **`vTaskStartScheduler()`** (Line ~2440)
   - 创建空闲任务 `prvIdleTask`
   - 如果 `configUSE_TIMERS = 1`，创建定时器服务任务
   - 调用 `xPortStartScheduler()` 启动调度器
   - **关键点**：此函数在启动第一个任务后不会再返回

2. **`prvAddCurrentTaskToDelayedList()`** (Line ~900)
   - 当任务调用 `vTaskDelay` 或等待超时操作时调用
   - 将任务从就绪列表移到延时列表
   - **关键点**：使用一个按唤醒时间排序的列表，SysTick 中断检查列表头部即可

3. **`vTaskSwitchContext()`** (Line ~2900)
   - 从就绪列表中选出优先级最高的任务
   - 如果使用时间片轮转，同等优先级使用轮转调度
   - **关键点**：此函数在 PendSV 中被调用，返回值决定下一个运行的任务

#### 思考题

1. 如果一个任务 `vTaskDelay(1)`，最少会阻塞多长时间？（答：1 个 tick = 1ms，但受调度器运行时机影响，实际可能为 1~2ms）
2. 空闲任务什么时候运行？它做了什么？（答：所有其他任务都阻塞时；它会调用空闲钩子、给任务释放内存等）

---

### 学习内容 1.2：上下文切换 — PendSV 深度解析

#### 理论知识

Cortex-M3 的 PendSV（Pendable Service Call）是 FreeRTOS 上下文切换的核心。它的优先级可编程为最低（在你的项目中为 15），这意味着它**不会抢占任何 ISR**。

切换流程：

```
SysTick 中断产生
    │
    ├─ HAL_IncTick()
    ├─ xPortSysTickHandler()
    │     └─ 检查是否需要抢占（更高优先级任务就绪？）
    │           └─ 是 → 挂起 PendSV (ICSR 寄存器 bit 28 写 1)
    │
SysTick 中断返回
    │
    ├─ 由于 PendSV 已挂起，异常处理器查看 PendSV 是否已就绪
    │
PendSV_Handler 执行 (由 port.c 中的汇编实现)
    │
    ├─ 1. 保存当前任务寄存器到当前任务的栈中
    │     R4-R11, PSP, EXC_RETURN
    │
    ├─ 2. vTaskSwitchContext() — 选择下一个要运行的任务
    │
    ├─ 3. 恢复下一任务的寄存器
    │
    └─ 4. BX LR — 异常返回，类似于跳转到新任务
```

#### 代码阅读作业

**`FreeRTOS/portable/GCC/ARM_CM3/port.c`** 中的 `xPortPendSVHandler` 汇编函数：

```
xPortPendSVHandler:
    mrs r0, psp            // 获取当前任务的栈指针（进程栈）
    isb

    // 保存 R4-R11
    stmdb r0!, {r4-r11}

    // 获取当前 TCB 指针
    ldr r3, =pxCurrentTCB
    ldr r2, [r3]
    str r0, [r2]           // 保存新栈指针到 TCB 的 pxTopOfStack

    // 保存 R4-R11 后，调用 C 函数选择下一个任务
    stmdb sp!, {r3, r14}
    mov r0, #configMAX_SYSCALL_INTERRUPT_PRIORITY
    msr basepri, r0        // 关中断（临界区）
    bl vTaskSwitchContext
    mov r0, #0
    msr basepri, r0        // 开中断
    ldmia sp!, {r3, r14}

    // 加载新任务的 TCB 和栈
    ldr r1, [r3]           // 读取新的 pxCurrentTCB
    ldr r0, [r1]           // 读取新任务的栈顶指针
    ldmia r0!, {r4-r11}    // 恢复 R4-R11

    msr psp, r0            // 更新 PSP
    isb
    bx r14                 // 异常返回，硬件自动弹出 R0-R3, R12, LR, PC, xPSR
```

#### 动手练习

在 `stm32f1xx_it.c` 的 `PendSV_Handler` 中，在跳转到 `xPortPendSVHandler` 之前，先通过 UART4 输出调试信息（**注意：** 这只是学习实验，不要在正式代码中这么做，因为每次上下文切换都打印会拖垮系统）：

```c
// PendSV 中打断点观察或计数
volatile uint32_t g_context_switch_count = 0;
```

**练习**：在项目中添加一个全局计数器 `g_context_switch_count`，在 `PendSV_Handler` 中递增。通过 CDC 串口输出每秒的上下文切换次数，分析不同负载下的切换频率。

---

### 学习内容 1.3：栈空间分析与优化

#### 理论知识

每个 FreeRTOS 任务都使用独立的栈。栈用于：
- 局部变量
- 函数调用时保存返回地址和寄存器
- 中断发生时保存上下文

当前项目的栈配置：

| 任务 | 栈大小(words) | 栈大小(bytes) | 实际使用 | 浪费 |
|------|--------------|--------------|---------|------|
| LVGL | 512 | 2048 | 待测量 | 待定 |
| FILE_SVC | 256 | 1024 | 待测量 | 待定 |
| CDC_Tx | 128 | 512 | 待测量 | 待定 |
| Timer Svc | 128 | 512 | 待测量 | 待定 |
| Idle | 128 | 512 | 待测量 | 待定 |
| **合计** | **1152** | **4608** | | |

#### 动手练习：测量实际栈使用

**步骤 1：添加栈水印 API 的包含**

`FreeRTOSConfig.h` 中确认以下宏已启用：
```c
#define INCLUDE_uxTaskGetStackHighWaterMark    1
```

**步骤 2：在 CDC_Tx 任务中输出栈信息**

在 `main.c` 的 `CdcTxTask` 中增加调试输出（后期可移除）：

```c
static void CdcTxTask(void *argument)
{
    (void)argument;
    char msg[64];

    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* ═══ 学习调试：每 100 次发送一次栈信息 ═══ */
        static uint32_t cnt = 0;
        if (++cnt % 100 == 0) {
            UBaseType_t highWaterMark;
            uint32_t freeHeap = xPortGetFreeHeapSize();

            highWaterMark = uxTaskGetStackHighWaterMark(NULL);  // 本任务
            snprintf(msg, sizeof(msg),
                "[CDC_TX] stack high water=%u words, heap free=%u\r\n",
                highWaterMark, freeHeap);
            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
        }
        /* ════════════════════════════════════════ */

        CDC_ProcessTx();
    }
}
```

**步骤 3：测量并优化**

编译 → 运行 → 观察 CDC 串口输出的数值。如果 `highWaterMark` 稳定在 60 以上（即实际只用了 68 words），说明 `128 words` 的栈可以缩减到 `100 words`，节省 112 字节 RAM。

`✶ Insight: 为什么要缩减栈？ ──────────────────────────`
STM32F103RE 只有 64KB RAM，其中 10KB 给 FreeRTOS heap，4.5KB 给栈，剩余给 HAL、USB 缓冲区、LVGL 显示缓冲区等。每节省 1KB，heap 就多 1KB，意味着可以创建更多任务或更大的队列。嵌入式开发中 RAM 是黄金。
`──────────────────────────────────────────────────────────`

---

### 学习内容 1.4：临界区与中断安全

#### 理论知识

FreeRTOS 提供了两套临界区保护机制：

**方式 1：`taskENTER_CRITICAL()` / `taskEXIT_CRITICAL()`**

```c
taskENTER_CRITICAL();
// 访问共享资源...
taskEXIT_CRITICAL();
```

原理：使用 `basepri` 寄存器屏蔽优先级低于 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 的所有中断。换言之，优先级 5~15 的中断被关，优先级 0~4 的中断（你的 TIM3）不受影响。

**方式 2：从 ISR 调用的 `FromISR` API**

项目中已使用的模式：
```c
// USB ISR 中：
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
vTaskNotifyGiveFromISR(g_cdc_tx_task_handle, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

原理：`FromISR` 函数会检查是否唤醒了更高优先级的任务，但不立即切换——它将优先级比较结果存入 `xHigherPriorityTaskWoken`，由调用方决定是否在 ISR 返回前触发 PendSV。

#### 现有代码分析

**场景 1：`g_tx_pending` 标志位是否安全？**

```c
// usbd_cdc_if.c — CDC_DataRx_FS (ISR 上下文)
if (g_tx_pending == 0) {        // ← 读
    g_tx_pending = 1;           // ← 写
    vTaskNotifyGiveFromISR(...);
}

// usbd_cdc_if.c — CDC_ProcessTx (任务上下文)
if (g_tx_pending) {             // ← 读
    // 发送数据...
    g_tx_pending = 0;           // ← 写
}
```

**分析**：`g_tx_pending` 是 `volatile` 变量，在多任务/中断环境中，ISR 写 + Task 读存在潜在的**竞争条件**。但这里 ISR 只做 "0→1" 的写入，Task 只做 "1→0" 的写入，所以实际上目前是安全的。不过更严谨的做法是在 Task 侧用临界区保护读取和清除操作。

#### 思考题

1. 为什么 `taskENTER_CRITICAL()` 很"重"？（答：因为它操作 `basepri` 寄存器，会整体关闭一大片中优先级，影响系统实时性）
2. 项目中哪些操作应该放在临界区中？（答：多任务共享的全局变量操作，如 `g_tx_pending` 的 read-modify-write 序列）

---

### 学习内容 1.5：系统启动流程完整追溯

#### 理论知识

从 `main()` 到第一个任务运行，FreeRTOS 经历以下步骤：

```
main()
    │
    ├─ HAL_Init() / SystemClock_Config()
    ├─ 外设初始化（GPIO, SPI, TIM, USB...）
    ├─ FATFS 初始化、W25QXX 识别
    ├─ LCD 初始化、LVGL 初始化
    │
    ├─ FS_Init()
    │     ├─ xQueueCreate() — 创建 FATFS 请求队列
    │     └─ xTaskCreate(FILE_SVC) — 创建文件服务任务
    │
    ├─ xTaskCreate(LvglTask)     — 创建 LVGL 任务（栈在 heap 中分配）
    ├─ xTaskCreate(CdcTxTask)    — 创建 CDC_TX 任务
    │
    └─ vTaskStartScheduler()
          │
          ├─ xPortStartScheduler()
          │     ├─ 设置 SysTick 中断周期（configCPU_CLOCK_HZ / configTICK_RATE_HZ）
          │     └─ 触发 SVC 异常
          │
          ├─ SVC_Handler → vPortSVCHandler (port.c)
          │     └─ 通过 SVC 异常返回的方式启动第一个任务
          │
          └─ [第一个任务开始运行]
                ├─ FILE_SVC 阻塞在 xQueueReceive()
                ├─ LVGL 开始渲染 → vTaskDelay(5) 阻塞
                ├─ CDC_TX 阻塞在 ulTaskNotifyTake()
                └─ Idle 任务运行（无应用代码时）
```

#### 代码阅读作业

**`FreeRTOS/portable/GCC/ARM_CM3/port.c`** 中的 `vPortSVCHandler`：

```c
vPortSVCHandler:
    ldr r3, =pxCurrentTCB    // 获取第一个任务的 TCB 指针
    ldr r1, [r3]             // 读取 TCB
    ldr r0, [r1]             // 读取任务的栈顶指针
    ldmia r0!, {r4-r11}      // 恢复 R4-R11
    msr psp, r0              // 设置进程栈指针
    isb
    mov r0, #0
    msr basepri, r0          // 确保中断全开
    bx r14                   // 异常返回 → 弹出第一个任务的 R0-R3, R12, LR, PC, xPSR
```

**关键理解**：这个函数并没有"调用"任务函数，而是通过异常返回机制**跳转**到任务函数。`bx r14` 执行时，CPU 自动从栈上弹出 PC（即任务函数的地址），然后开始执行。这就是为什么任务函数看起来像普通的函数，但实际上是被异常返回"启动"的。

---

### 第一阶段验证清单

完成第一阶段学习后，你应该能：

- [ ] 画出 FreeRTOS 任务状态转移图并解释各状态切换条件
- [ ] 说出 `vTaskStartScheduler()` 执行后系统的完整启动流程
- [ ] 解释 PendSV 为什么用最低优先级，以及上下文切换的完整汇编流程
- [ ] 用 `uxTaskGetStackHighWaterMark()` 测量并优化至少一个任务的栈大小
- [ ] 解释项目中 `g_tx_pending` 标志位的安全性和潜在风险
- [ ] 区分临界区保护和 `FromISR` API 两种中断安全编程方式
- [ ] 理解 `basepri` 寄存器在临界区中的作用

---

## 第二阶段：IPC 进阶 — 同步与通信

### 阶段目标

- 掌握 FreeRTOS 所有 IPC 机制（互斥量、信号量、事件组）
- 理解优先级反转问题及互斥量的优先级继承解决方案
- 能用事件组实现多条件等待模式

### 学习内容 2.1：互斥量 (Mutex) 与优先级继承 (待实现)

#### 理论知识

互斥量本质上是启用了**优先级继承**的二进制信号量。

**问题：优先级反转**
```
低优先级任务 L 持有互斥量
    → 中优先级任务 M 抢占 L（L 还没释放互斥量）
    → 高优先级任务 H 需要互斥量，但被 L 持有，只能等待
    → M 继续执行，H 被阻塞等待
    → 实际上优先级颠倒了：H 在等 M，但 M 优先级比 H 低
```

**解决方案：优先级继承**
```
L 持有互斥量时，临时"继承"等待该互斥量的最高优先级任务的优先级
    → L 被提升到 H 的优先级
    → M 无法再抢占 L（因为 L 现在优先级比 M 高）
    → L 尽快运行 → 释放互斥量 → 恢复原优先级
    → H 获得互斥量 → 运行
```

#### 动手练习：LCD 缓冲区互斥量保护

**步骤 1：在 `FreeRTOSConfig.h` 中启用互斥量**
```c
#define configUSE_MUTEXES    1
```

**步骤 2：在 lvgl_ui.c 中创建和使用互斥量**

```c
/* lvgl_ui.c */
#include "semphr.h"

static SemaphoreHandle_t lcd_mutex = NULL;

// 在初始化中创建
void lcd_mutex_init(void) {
    lcd_mutex = xSemaphoreCreateMutex();
}

// 刷屏时加锁
void lcd_flush_with_lock(...) {
    xSemaphoreTake(lcd_mutex, portMAX_DELAY);
    // 实际刷屏代码
    xSemaphoreGive(lcd_mutex);
}
```

### 学习内容 2.2：计数信号量 (待实现)

适合生产者-消费者模型。

**练习**：为 USB CDC RX 数据使用计数信号量代替简单的标志位，支持缓冲多个数据包。

### 学习内容 2.3：事件组 (待实现)

**练习**：系统状态监控任务

```c
EventGroupHandle_t xSystemEvents;
#define EVENT_USB_READY    (1 << 0)
#define EVENT_FS_MOUNTED   (1 << 1)
#define EVENT_CDC_DATA     (1 << 2)

/* 监控任务 — 等待 USB 和 FS 都就绪后才开始工作 */
void SystemMonitorTask(void *arg) {
    EventBits_t bits = xEventGroupWaitBits(
        xSystemEvents,
        EVENT_USB_READY | EVENT_FS_MOUNTED,  // 等待这两个事件
        pdTRUE,                              // 等待到后自动清除
        pdTRUE,                              // 需要全部满足（逻辑 AND）
        portMAX_DELAY
    );
    // USB 和 FS 都已就绪，开始工作...
}
```

---

## 第三阶段：实时模式 — 定时器和精确周期

### 阶段目标

- 掌握 FreeRTOS 软件定时器的使用场景
- 理解 `vTaskDelay` 和 `xTaskDelayUntil` 的区别
- 能用 `xTaskDelayUntil` 实现精确周期任务

### 学习内容 3.1：软件定时器

**练习**：Uptime 心跳输出（通过 CDC 串口每秒输出一次系统运行时间）

```c
#include "timers.h"

static void UptimeTimerCallback(TimerHandle_t xTimer) {
    char buf[48];
    TickType_t now = xTaskGetTickCount();
    snprintf(buf, sizeof(buf), "Uptime: %lu ms, Heap: %u\r\n",
             now, xPortGetFreeHeapSize());
    CDC_Transmit_FS((uint8_t*)buf, strlen(buf));
}

// 在 main() 中创建：
TimerHandle_t tmr = xTimerCreate(
    "UPTIME",                 // 定时器名称
    pdMS_TO_TICKS(1000),      // 周期 1000ms
    pdTRUE,                   // 自动重载
    NULL,                     // 定时器 ID
    UptimeTimerCallback       // 回调函数
);
xTimerStart(tmr, 0);          // 启动（0 表示不等待）
```

### 学习内容 3.2：xTaskDelayUntil vs vTaskDelay

**对比**：
```c
// 相对延时 — 有累积误差
vTaskDelay(pdMS_TO_TICKS(5));  // "从现在起 5ms 后"
// 实际醒来 = 当前时间 + 5ms + 任务切换开销 + 中断开销

// 绝对延时 — 固定在 tick 周期上
TickType_t xLastWakeTime = xTaskGetTickCount();
vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(5));
// xLastWakeTime 会自动更新，每次都在同一个 tick 边界醒来
```

**练习**：改造 LVGL 任务使用 `xTaskDelayUntil`

---

## 第四阶段：调试与诊断

### 阶段目标

- 部署栈溢出检测和内存分配失败钩子
- 部署运行时统计，量化 CPU 利用率
- 能用 `configASSERT` 做运行时断言检查

### 学习内容 4.1：栈溢出检测

```c
#define configCHECK_FOR_STACK_OVERFLOW  2
#define configUSE_MALLOC_FAILED_HOOK    1

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // 通过 UART4 输出错误，然后停机
    char msg[] = "STACK OVERFLOW: ";
    HAL_UART_Transmit(&huart4, (uint8_t*)msg, strlen(msg), 100);
    HAL_UART_Transmit(&huart4, (uint8_t*)pcTaskName, strlen(pcTaskName), 100);
    __disable_irq();
    while(1);
}
```

### 学习内容 4.2：运行时统计

```c
#define configGENERATE_RUN_TIME_STATS    1
#define configUSE_STATS_FORMATTING_FUNCTIONS  1
// 使用 TIM2 或 TIM4 作为高精度计时器
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()  // 配置 TIM
#define portGET_RUN_TIME_COUNTER_VALUE()          // 读 TIM 计数值
```

调用 `vTaskGetRunTimeStats((char *)buf)` 获取类似如下的输出：

```
Task         State  Priority  Runtime(us)   Runtime(%)
LVGL         X      2         12345678      45.2
CDC_TX       B      3         2345678       8.6
FILE_SVC     B      1         123456        0.5
IDLE         R      0         12345678      45.7
```

---

## 第五阶段：综合实战 — 系统监控仪表盘

### 阶段目标

综合运用前四个阶段的知识，在 LCD 上构建一个实时的 FreeRTOS 系统状态监控页面。

### 功能设计

在 LVGL 中增加一个"System Info"页面，显示：

1. **任务列表** — 任务名、优先级、状态、栈利用率
2. **CPU 利用率** — 每个任务的 CPU 占比（从运行时统计数据解析）
3. **Heap 使用** — 已用/总量及百分比
4. **系统运行时间** — 天:时:分:秒

### 技术要点

- 使用 `uxTaskGetSystemState()` 获取所有任务信息
- 使用 `xPortGetFreeHeapSize()` 获取堆剩余
- 使用 `xTaskGetTickCount()` 计算运行时间
- 使用 LVGL 的 `lv_table` 和 `lv_bar` 组件显示

---

## 附录：关键 API 速查

### 任务管理

| API | 功能 | 注意 |
|-----|------|------|
| `xTaskCreate()` | 创建任务 | 参数：函数、名、栈深度(words)、参数、优先级、句柄 |
| `vTaskDelete(NULL)` | 删除自身 | 删除后需要 `vTaskDelay(0)` 立即让出 CPU |
| `vTaskDelay(ms)` | 相对延时阻塞 | 参数是 tick 数，用 `pdMS_TO_TICKS(ms)` 转换 |
| `vTaskDelayUntil(&prev, inc)` | 绝对延时阻塞 | 精确周期，prev 会被自动更新 |
| `uxTaskGetStackHighWaterMark(NULL)` | 最小剩余栈(本任务) | 输入 NULL 表示当前任务 |
| `xTaskGetTickCount()` | 获取当前 tick 计数值 | 从任务中调用 |
| `xTaskGetTickCountFromISR()` | ISR 中获取当前 tick | 从 ISR 中调用 |

### 队列

| API | 功能 | 注意 |
|-----|------|------|
| `xQueueCreate(len, item_size)` | 创建队列 | 参数：深度、元素大小(字节) |
| `xQueueSend(queue, item, timeout)` | 发送(尾) | 数据被拷贝入队列 |
| `xQueueReceive(queue, buf, timeout)` | 接收 | 数据从队列拷贝到 buf |
| `xQueueSendFromISR()` | ISR 中发送 | 需要 `pxHigherPriorityTaskWoken` 参数 |

### IPC

| API | 功能 | 注意 |
|-----|------|------|
| `xSemaphoreCreateBinary()` | 二进制信号量 | 需要 `configUSE_SEMAPHORES` |
| `xSemaphoreCreateMutex()` | 互斥量(优先级继承) | 需要 `configUSE_MUTEXES` |
| `xSemaphoreCreateCounting(max, init)` | 计数信号量 | 需要 `configUSE_COUNTING_SEMAPHORES` |
| `xEventGroupCreate()` | 事件组 | 24 个可用位(bit 0~23) |
| `xEventGroupWaitBits()` | 等待事件组位 | 支持 AND/OR 模式 |

### 任务通知

| API | 功能 | 注意 |
|-----|------|------|
| `xTaskNotifyGive(task)` | 任务中发通知(递增值) | 略微提升通知值 |
| `vTaskNotifyGiveFromISR(task, &woken)` | ISR 中发通知 | 需要 `&xHigherPriorityTaskWoken` |
| `ulTaskNotifyTake(clear, timeout)` | 等待并消费通知 | `clear=pdTRUE` 清零通知值 |
| `xTaskNotifyWait(clear, set, &val, timeout)` | 等待通知(完整版) | 可指定清除/设置哪些位 |

### 定时器

| API | 功能 | 注意 |
|-----|------|------|
| `xTimerCreate(name, period, auto_reload, id, cb)` | 创建软件定时器 | 回调在定时器服务任务中执行 |
| `xTimerStart(timer, block_time)` | 启动定时器 | `block_time` 等待命令队列的 tick 数 |
| `xTimerStop(timer, block_time)` | 停止定时器 | |

### 工具

| API | 功能 | 注意 |
|-----|------|------|
| `xPortGetFreeHeapSize()` | 获取剩余堆大小 | 调试用 |
| `uxTaskGetSystemState(arr, max, &total)` | 获取所有任务状态 | 需要 `configUSE_TRACE_FACILITY` |
| `vTaskGetRunTimeStats(buf)` | CPU 利用率统计 | 需要 `configGENERATE_RUN_TIME_STATS` |
| `configASSERT(x)` | 运行时断言 | 需要 `configASSERT_DEFINED` |
