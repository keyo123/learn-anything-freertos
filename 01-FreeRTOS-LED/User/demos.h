#ifndef __DEMOS_H
#define __DEMOS_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* ========== 全局公用内核对象 ========== */
extern SemaphoreHandle_t xPrintfMutex;

/* ========== printf 线程安全宏 ========== */
#define printfSafe(...) do { \
    if (xPrintfMutex != NULL) { \
        xSemaphoreTake(xPrintfMutex, portMAX_DELAY); \
    } \
    printf(__VA_ARGS__); \
    if (xPrintfMutex != NULL) { \
        xSemaphoreGive(xPrintfMutex); \
    } \
} while(0)

/* ========== 各模块演示启动入口 ========== */

/* 互斥量演示 (包含：基础互斥、优先级继承、递归互斥量、临界区对比) */
void start_mutex_demos(void);

/* 事件组演示 (温控系统模拟) */
void start_event_demos(void);

/* 二值与计数信号量演示 (包含：中断同步、流水线事件传递、信号量优先级继承对比) */
void start_semphr_demos(void);

/* 动态任务创建与删除演示 */
void start_task_demo(void);

/* 中断安全 API 演示 */
void start_isr_demo(void);

/* 递归互斥量演示 */
void start_rec_mutex_demo(void);

#endif /* __DEMOS_H */
