#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * 学习点：FreeRTOSConfig.h 是 FreeRTOS 唯一必须的配置头文件
 * 所有配置宏都在这里定义，内核源码根据它们编译出对应的功能
 *----------------------------------------------------------*/

/* STM32F103RE 系统时钟 = 72MHz（HSE 8MHz × PLLMUL9）*/
#define configCPU_CLOCK_HZ              72000000

/* RTOS tick 频率 = 1000Hz（= 1ms 一次 tick 中断）
 * 频率越高，时间精度越高，但上下文切换开销越大 */
#define configTICK_RATE_HZ              1000

/* FreeRTOS 内核堆大小（字节）
 * 所有任务栈、TCB、信号量、队列等内核对象的内存都由这个堆分配
 * 当前已有 LVGL(2KB栈) + CDC_Tx(0.5KB栈) + FileSvc(1KB栈) 三个任务 */
#define configTOTAL_HEAP_SIZE           ((size_t)(10 * 1024))

/* 任务栈深度单位 — word（Cortex-M3 1 word = 4 字节）
 * 空闲任务使用这个大小 */
#define configMINIMAL_STACK_SIZE        ((unsigned short)128)

/* 任务名最大长度（含 '\0'）*/
#define configMAX_TASK_NAME_LEN         (16)

/* 最大优先级数 — 0~(configMAX_PRIORITIES-1)
 * 数值越大优先级越高。空闲任务占 0 */
#define configMAX_PRIORITIES            (5)

/* 抢占式调度 — FreeRTOS 默认行为：高优先级任务就绪时立即抢占低优先级任务 */
#define configUSE_PREEMPTION            1

/* 时间片轮转 — 同等优先级任务轮流执行 */
#define configUSE_TIME_SLICING          1

/* Tick 类型宽度 — Cortex-M3 是 32 位架构，使用 32 位 tick */
#define configTICK_TYPE_WIDTH_IN_BITS   TICK_TYPE_WIDTH_32_BITS

/*-----------------------------------------------------------
 * 功能开关
 *----------------------------------------------------------*/

/* 软件定时器 — LVGL 内部定时机制需要
 * 启用后会创建一个定时器守护任务（优先级 configTIMER_TASK_PRIORITY）*/
#define configUSE_TIMERS                1
#define configTIMER_TASK_PRIORITY       (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH        10
#define configTIMER_TASK_STACK_DEPTH    configMINIMAL_STACK_SIZE

/* Tick 钩子 — 不需要，LVGL 由 TIM3 直接提供 1ms tick */
#define configUSE_TICK_HOOK             0

/* 空闲钩子  — 不需要 */
#define configUSE_IDLE_HOOK             0
#define configUSE_MALLOC_FAILED_HOOK    0

/*-----------------------------------------------------------
 * 中断优先级配置（Cortex-M3 关键概念）
 *
 * STM32F103 使用 4 位优先级（NVIC_PRIO_BITS = 4），共 16 级
 * 数值越小 → 优先级越高（硬件特性）
 *
 * configLIBRARY_KERNEL_INTERRUPT_PRIORITY:
 *   内核自身的中断优先级 —— 必须设为最低（15）
 *   因为 SysTick 和 PendSV 中断使用这个优先级
 *
 * configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY:
 *   可调用 FreeRTOS API 的最高中断优先级（设为 5）
 *   优先级 0~4 的中断 → 不可调用 FreeRTOS API
 *   优先级 5~15 的中断 → 可以调用 xQueueSendFromISR 等
 *----------------------------------------------------------*/
#define configLIBRARY_KERNEL_INTERRUPT_PRIORITY         15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5
#define configKERNEL_INTERRUPT_PRIORITY     (configLIBRARY_KERNEL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - __NVIC_PRIO_BITS))

/* STM32F103 使用 4 位中断优先级（在 CMSIS core_cm3.h 中定义，但
 * FreeRTOSConfig.h 先于 CMSIS 包含，因此在此处预先定义）；
 * 如果 CMSIS 尚未包含则提供默认值 */
#ifndef __NVIC_PRIO_BITS
#define __NVIC_PRIO_BITS                  4
#endif

/* 中断处理器安装检查：我们使用桥接方式（SVC_Handler → vPortSVCHandler），
 * 向量表指向的是 SVC_Handler 而非 vPortSVCHandler，因此关掉此检查 */
#define configCHECK_HANDLER_INSTALLATION   0
#define configASSERT_DEFINED               0

/*-----------------------------------------------------------
 * 不需要的功能 — 关闭以减小内核体积和 RAM 消耗
 *----------------------------------------------------------*/
#define configUSE_QUEUE_SETS             0
#define configUSE_TASK_NOTIFICATIONS     1
#define configSUPPORT_STATIC_ALLOCATION  0  /* 全部使用动态分配 */
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configUSE_CO_ROUTINES            0
#define configUSE_MUTEXES                1
#define configUSE_RECURSIVE_MUTEXES      0
#define configUSE_COUNTING_SEMAPHORES    1
#define configUSE_APPLICATION_TASK_TAG   0
#define configUSE_POSIX_ERRNO            0

/*-----------------------------------------------------------
 * 可选的函数 — 保持默认值
 *----------------------------------------------------------*/
#define INCLUDE_vTaskPrioritySet         0
#define INCLUDE_uxTaskPriorityGet        0
#define INCLUDE_vTaskDelete              1
#define INCLUDE_vTaskSuspend             1
#define INCLUDE_xTaskDelayUntil          1
#define INCLUDE_vTaskDelay               1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_xSemaphoreGetMutexClient  0

/*-----------------------------------------------------------
 * ARM Compiler (AC5/AC6) 需要的外部函数声明
 *----------------------------------------------------------*/
extern void vPortSVCHandler(void);   // SVC 异常处理 — 用于启动第一个任务
extern void xPortPendSVHandler(void); // PendSV 异常处理 — 用于上下文切换
extern void xPortSysTickHandler(void); // SysTick 异常处理 — RTOS tick

#endif /* FREERTOS_CONFIG_H */
