/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
     ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f10x.h"
#include "bsp_usart.h"

// 针对不同的编译器选择不同的 stdint.h 文件
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif

// 断言
#define vAssertCalled(char, int) printf("Error:%s,%d\r\n", char, int)
#define configASSERT(x)     if ((x) == 0)           vAssertCalled(__FILE__, __LINE__)

/************************************************************************
 *               FreeRTOS基础配置选型
 *********************************************************************/
/* 置1时RTOS使用抢占式调度器，置0时RTOS使用协程式调度器（时间片轮转）。
 *
 * 协程式调度系统中，任务只有在主动释放CPU或者被更高优先级的任务抢占才进行切换。
 * 任务切换的时间完全取决于任务本身的执行情况。
 */
#define configUSE_PREEMPTION 1

// 置1使用时间片轮转调度(默认是开启的)
#define configUSE_TIME_SLICING 1

/* 某些运行FreeRTOS的硬件有两种方法选择下一个要执行的任务：
 * 通用方法和特定于硬件的方法（以下简称“特殊方法”）。
 *
 * 通用方法：
 *      1.configUSE_PORT_OPTIMISED_TASK_SELECTION 为 0 或者硬件不支持这种特殊方法。
 *      2.可以用于所有FreeRTOS支持的硬件。
 *      3.完全由C语言实现，效率相对特殊方法低。
 *      4.不强求限制最大任务优先级。
 * 特殊方法：
 *      1.必须将configUSE_PORT_OPTIMISED_TASK_SELECTION设置为1。
 *      2.依赖一个或多个特定架构的汇编指令（一般是类似计算前导零[CLZ]指令）。
 *      3.比通用方法更高效。
 *      4.一般强制限制最大任务优先级为32。
 */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* 置1时使能低功耗tickless模式；为0时保持系统节拍（tick）中断一直运行。
 * 开启此功能需要硬件支持低功耗模式，并在进入睡眠后有唤醒机制。
 */
#define configUSE_TICKLESS_IDLE 0

/* 写入实际的CPU内核主频频率，也就是CPU指令执行频率，通常称为Fclk。
 * 比如在STM32F103上通常为主频 72MHz（SystemCoreClock）。
 */
#define configCPU_CLOCK_HZ (SystemCoreClock)

// RTOS系统节拍中断频率，即一秒钟产生的中断次数，通常为1000次/秒（即1ms一个节拍）
#define configTICK_RATE_HZ ((TickType_t)1000)

// 可使用的最大任务优先级数量
#define configMAX_PRIORITIES (32)

// 空闲任务使用的堆栈大小
#define configMINIMAL_STACK_SIZE ((unsigned short)128)

// 任务名字的最大长度上限
#define configMAX_TASK_NAME_LEN (16)

// 系统节拍计数器的数据类型。1为16位无符号整型，0为32位无符号整型
#define configUSE_16_BIT_TICKS 0

// 空闲任务是否让出CPU使用权给同等优先级的其他任务
#define configIDLE_SHOULD_YIELD 1

// 队列集功能使能
#define configUSE_QUEUE_SETS 0

// 任务通知功能使能，默认开启
#define configUSE_TASK_NOTIFICATIONS 1

// 互斥量功能使能
#define configUSE_MUTEXES 1

// 递归互斥量功能使能
#define configUSE_RECURSIVE_MUTEXES 1

// 计数信号量功能使能
#define configUSE_COUNTING_SEMAPHORES 1

// 注册信号量和队列的最大数量
#define configQUEUE_REGISTRY_SIZE 10

#define configUSE_APPLICATION_TASK_TAG 0

/*****************************************************************
              FreeRTOS内存管理有关配置
*****************************************************************/
// 支持动态内存分配
#define configSUPPORT_DYNAMIC_ALLOCATION 1
// 支持静态内存分配
#define configSUPPORT_STATIC_ALLOCATION 0
// 系统所有任务占用的总堆（Heap）大小
#define configTOTAL_HEAP_SIZE ((size_t)(10 * 1024))

/***************************************************************
             FreeRTOS钩子函数有关的配置
**************************************************************/
// 空闲任务钩子函数使能
#define configUSE_IDLE_HOOK 0

// 时间片中断钩子函数使能
#define configUSE_TICK_HOOK 0

// 内存分配失败钩子函数使能
#define configUSE_MALLOC_FAILED_HOOK 1

// 栈溢出检测使能。0为关闭，1或2为开启
#define configCHECK_FOR_STACK_OVERFLOW 2

/********************************************************************
          FreeRTOS运行时间和任务状态统计配置
**********************************************************************/
// 运行时间统计功能使能
#define configGENERATE_RUN_TIME_STATS 0
// 可视化跟踪辅助使能
#define configUSE_TRACE_FACILITY 0
/* 当configUSE_TRACE_FACILITY同时为1时，才支持以下3个函数：
 * prvWriteNameToBuffer()
 * vTaskList(),
 * vTaskGetRunTimeStats()
 */
#define configUSE_STATS_FORMATTING_FUNCTIONS 1

/********************************************************************
                FreeRTOS协程有关配置
*********************************************************************/
// 启用协程功能
#define configUSE_CO_ROUTINES 0
// 协程的最大优先级
#define configMAX_CO_ROUTINE_PRIORITIES (2)

/***********************************************************************
                FreeRTOS软件定时器有关配置
**********************************************************************/
// 启用软件定时器
#define configUSE_TIMERS 1
// 软件定时器任务的优先级
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)
// 软件定时器命令队列长度
#define configTIMER_QUEUE_LENGTH 10
// 软件定时器任务的堆栈深度
#define configTIMER_TASK_STACK_DEPTH (configMINIMAL_STACK_SIZE * 2)

/************************************************************
            FreeRTOS可选API函数使能配置
************************************************************/
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskCleanUpResources 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_eTaskGetState 1
#define INCLUDE_xTimerPendFunctionCall 0
// #define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
// #define INCLUDE_xTaskGetIdleTaskHandle          0

/******************************************************************
            FreeRTOS中断配置有关选项
******************************************************************/
#ifdef __NVIC_PRIO_BITS
#define configPRIO_BITS __NVIC_PRIO_BITS
#else
#define configPRIO_BITS 4
#endif
// 中断最低优先级
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15

// 系统可管理的最高中断优先级
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS)) /* 240 */

#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/****************************************************************
            FreeRTOS中断服务函数重定义
****************************************************************/
#define xPortPendSVHandler PendSV_Handler
#define vPortSVCHandler SVC_Handler

/* 跟踪工具相关配置 */
#if (configUSE_TRACE_FACILITY == 1)
#include "trcRecorder.h"
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#endif

#endif /* FREERTOS_CONFIG_H */
