# FreeRTOS 知识图谱

## 1. RTOS 基础概念
- **什么是 RTOS** — 实时操作系统 vs 通用操作系统，硬实时 vs 软实时
- **FreeRTOS 概述** — 历史、特点、支持的架构、许可证
- **任务 (Task)** — 任务的概念、状态机（运行/就绪/阻塞/挂起）
- **调度机制** — 抢占式调度、时间片轮转、协作式调度
- **任务优先级** — 优先级分配策略、优先级反转问题

## 2. 任务管理
- **任务创建与删除** — `xTaskCreate`、`xTaskCreateStatic`、`vTaskDelete`
- **任务控制** — `vTaskDelay`、`vTaskDelayUntil`、任务挂起/恢复
- **任务通知** — 轻量级 IPC，`xTaskNotifyGive`、`ulTaskNotifyTake`
- **空闲任务** — 空闲钩子、低功耗 Tickless 模式
- **任务堆栈** — 栈深度估算、`uxTaskGetStackHighWaterMark`

## 3. 队列与 IPC
- **队列 (Queue)** — 创建、发送、接收、阻塞超时
- **队列集 (Queue Set)** — 多队列多事件源管理
- **流缓冲区 (Stream Buffer)** — 字节流传输
- **消息缓冲区 (Message Buffer)** — 变长消息传输
- **事件组 (Event Group)** — 事件标志、多任务同步

## 4. 同步机制
- **二值信号量** — 任务同步、中断同步
- **计数信号量** — 资源管理、事件计数
- **互斥量 (Mutex)** — 优先级继承、死锁预防
- **递归互斥量** — 递归函数中的互斥保护

## 5. 软件定时器
- **定时器概念** — 自动重载 vs 一次性
- **定时器回调** — 守护任务、回调上下文
- **定时器 API** — `xTimerCreate`、`xTimerStart`、`xTimerReset`

## 6. 中断管理
- **中断安全 API** — `FromISR` 函数族
- **延迟中断处理** — 上半部/下半部模式
- **嵌套中断** — 中断优先级配置、configMAX_SYSCALL_INTERRUPT_PRIORITY
- **临界区** — `taskENTER_CRITICAL`、`taskEXIT_CRITICAL`、暂停调度器

## 7. 内存管理
- **heap 方案对比** — heap_1 ~ heap_5 的适用场景
- **动态 vs 静态分配** — `xTaskCreate` vs `xTaskCreateStatic`
- **内存碎片** — 避免策略、heap_4 的最佳匹配算法

## 8. 低功耗与 Tickless
- **Tickless 模式** — `configUSE_TICKLESS_IDLE`
- **睡眠与停止模式** — MCU 功耗优化
- **定时器补偿** — Tickless 退出后的时间补偿

## 9. 调试与优化
- **运行时统计** — `vTaskGetRunTimeStats`、`uxTaskGetSystemState`
- **栈溢出检测** — `configCHECK_FOR_STACK_OVERFLOW`
- **Trace 功能** — FreeRTOS+Trace、SEGGER SystemView
- **性能分析** — 任务 CPU 占用、上下文切换频率

## 10. 项目实践
- **移植 FreeRTOS** — 新建项目、配置 FreeRTOSConfig.h
- **多任务应用架构** — 任务拆分、优先级设计、通信模式
- **常见问题** — 优先级反转(续)、死锁、栈溢出、看门狗集成
- **FreeRTOS+生态** — FreeRTOS+FAT、FreeRTOS+TCP、AWS IoT 集成
