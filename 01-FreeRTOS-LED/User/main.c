/**
 *********************************************************************
 * @file    main.c
 * @author  fire
 * @version V1.0
 * @date    2018-xx-xx
 * @brief   FreeRTOS v9.0.0 + STM32 LED + Demos Entry
 *********************************************************************
 */

/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"
/* 开发板硬件bsp头文件 */
#include "bsp_exti.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_usart.h"
/* 演示项目头文件 */
#include "demos.h"

/* 定义公用的 printf 安全锁 */
SemaphoreHandle_t xPrintfMutex = NULL;

/* LED1 任务句柄，兼容中断服务程序 stm32f10x_it.c 中的引用 */
TaskHandle_t LED1_Task_Handle = NULL;

static TaskHandle_t AppTaskCreate_Handle = NULL;

static void AppTaskCreate(void); /* 用于创建应用演示任务 */
static void BSP_Init(void);      /* 用于初始化板载相关资源 */

int main(void) {
  BaseType_t xReturn = pdPASS;

  /* 开发板硬件初始化 */
  BSP_Init();

  /* 创建全局的 printf 安全锁 */
  xPrintfMutex = xSemaphoreCreateMutex();

  /* 创建AppTaskCreate任务以初始化演示程序 */
  xReturn =
      xTaskCreate((TaskFunction_t)AppTaskCreate, (const char *)"AppTaskCreate",
                  (uint16_t)512, (void *)NULL, (UBaseType_t)1,
                  (TaskHandle_t *)&AppTaskCreate_Handle);

  if (pdPASS == xReturn) {
    /* 启动任务，开启调度 */
    vTaskStartScheduler();
  } else {
    return -1;
  }

  while (1)
    ; /* 正常不会执行到这里 */
}

static void AppTaskCreate(void) {
  taskENTER_CRITICAL(); // 进入临界区

  printfSafe("System Init Success!\r\n");

  /* ==================== 启动演示模块 ==================== */

  // /* 1. 启动信号量及优先级继承对比演示 */
  // start_semphr_demos();

  // /* 2. 启动互斥量、优先级继承与递归锁演示 */
  // start_mutex_demos();

  // /* 3. 启动事件组温控演示 */
  // start_event_demos();

  // /* 4. 启动任务创建与删除演示 */
  // start_task_demo();

  // /* 5. 启动中断安全 API 演示 */
  // start_isr_demo();

  // /* 6. 启动递归互斥量演示 */
  // start_rec_mutex_demo();

  // /* 7. 启动队列数据通信演示 */
  // start_queue_demo();

  // /* 8. 启动高级任务通知演示 */
  // start_notify_adv_demo();

  /* 9. 启动软件定时器演示 */
  start_timer_demo();

  /* ===================================================== */

  vTaskDelete(AppTaskCreate_Handle); // 删除AppTaskCreate初始化任务

  taskEXIT_CRITICAL(); // 退出临界区
}

static void BSP_Init(void) {
  /* STM32中断优先级分组为4 */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

  /* LED、串口及按键外部中断初始化 */
  LED_GPIO_Config();
  USART_Config();
  EXTI_Key_Config();
}
