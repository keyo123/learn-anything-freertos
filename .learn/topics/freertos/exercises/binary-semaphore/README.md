# 二值信号量 (Binary Semaphore)

## 🎯 目标
在 STM32 + FreeRTOS 硬件平台上，通过按键中断驱动三个练习，深入理解二值信号量的核心概念和典型应用场景。

## 📋 背景
你已经学过 Mutex，知道它用于保护共享资源（具有优先级继承、所有权等特性）。二值信号量看起来和 Mutex 很像（都只有两个状态），但设计哲学完全不同：

| 特性 | 二值信号量 | Mutex |
|------|-----------|-------|
| 创建后初始状态 | empty（不可用） | full（可用） |
| 优先级继承 | ❌ 无 | ✅ 有 |
| 所有权 | ❌ 任何人都能 Give | ✅ 谁 Take 谁 Give |
| 中断中 Give | ✅ `xSemaphoreGiveFromISR` | ❌ 禁止 |
| 典型用途 | 事件通知 / 任务同步 | 互斥访问 |

**二值信号量的核心思想：它是一个"旗帜（flag）"，不是一个"锁（lock）"。**

## ✅ 练习结构

### 练习 7.1 — ISR → Task 同步【最基本模式】
- 按 KEY1 → EXTI 中断 → `xSemaphoreGiveFromISR()` → 等待的任务被唤醒 → 切换 LED1
- **重点理解**：中断后半部（Bottom Half）模式 — ISR 只做最快的事（Give），繁重的处理留给任务

### 练习 7.2 — 双信号量 Pipeline
- KEY1 → 信号量 A → StageA 任务（LED2 亮）→ 信号量 B → StageB 任务（LED2 灭）
- **重点理解**：信号量可以在任务之间传递"事件"，形成处理流水线

### 练习 7.3 — 对比实验（二值信号量 vs Mutex 保护资源）
- 把练习 5 的 Mutex 换成二值信号量，观察保护效果
- **重点理解**：没有优先级继承时高优先级任务被阻塞的后果

## 💡 关键知识点

### 创建后是 empty！
```c
SemaphoreHandle_t xSem = xSemaphoreCreateBinary();
// xSem 初始为 empty！必须先 Give 一次才能被 Take
```
对比 Mutex：
```c
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
// xMutex 初始为 full，可以直接 Take
```

### ISR 安全 API
```c
// 在中断中使用 GiveFromISR（带上下文切换请求）
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xSemaphoreGiveFromISR(xSem, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

### Give/Take 配对无限制
二值信号量不像 Mutex 那样要求谁 Take 谁 Give。TaskA 可以 Take，ISR 可以 Give。这是它比 Mutex 更适合做事件通知的根本原因。

## 🔧 需要修改的文件
- `01-FreeRTOS-LED/User/starter.c` — 练习 7 的代码（已添加，补全 TODO）
- `01-FreeRTOS-LED/User/stm32f10x_it.c` — ISR 中已添加 GiveFromISR 支持

## 📎 相关概念
- 互斥量 Mutex — 对比学习，理解本质区别
- 队列 Queue — 二值信号量本质上就是长度为 1 的队列
- 计数信号量 — 二值信号量的推广（计数值 > 1）
